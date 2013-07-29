#ifndef QBRT_SCHEDULE_H
#define QBRT_SCHEDULE_H

#include "qbrt/core.h"
#include "qbrt/function.h"
#include <set>
#include <list>
#include <pthread.h>


typedef uint8_t CodeFrameType;
#define CFT_CALL	0
#define CFT_TAILCALL	1
#define CFT_LOCAL_FORK	2
#define CFT_REMOTE_FORK	3

typedef uint8_t CodeFrameState;
#define CFS_NEW		0
#define CFS_READY	1
#define CFS_IOWAIT	2
#define CFS_PEERWAIT	3
#define CFS_COMPLETE	4
#define CFS_FAILED	5

typedef uint32_t WorkerID; // this should just be OS thread id?

struct ParallelPath;
struct FunctionCall;
struct ProcessRoot;
struct StreamIO;
struct Module;
struct Application;
typedef std::map< std::string, const Module * > ModuleMap;


struct Pipe
{
public:
	Pipe()
	: pipe_lock()
	{
		pthread_spin_init(&pipe_lock, PTHREAD_PROCESS_PRIVATE);
	}

	bool empty() const;
	void push(qbrt_value *);
	qbrt_value * pop();

private:
	std::list< qbrt_value * > data;
	pthread_spinlock_t pipe_lock;
};

struct CodeFrame
: public qbrt_value_index
{
	ProcessRoot *proc;
	CodeFrame *parent;
	std::set< CodeFrame * > fork;
	StreamIO *io;
	CodeFrameType cftype;
	CodeFrameState cfstate;
	int pc;

	CodeFrame(CodeFrame &parent, CodeFrameType type)
	: proc(parent.proc)
	, parent(&parent)
	, io(NULL)
	, cftype(type)
	, cfstate(CFS_READY)
	, pc(0)
	, frame_context()
	{}

	CodeFrame(CodeFrameType type)
	: proc(NULL)
	, parent(NULL)
	, io(NULL)
	, cftype(type)
	, cfstate(CFS_READY)
	, pc(0)
	, frame_context()
	{}

	virtual FunctionCall & function_call() = 0;
	virtual const FunctionCall & function_call() const = 0;
	inline qbrt_value & reg(uint8_t);
	void io_push(StreamIO *s)
	{
		cfstate = CFS_IOWAIT;
		io = s;
	}
	void io_pop();

	virtual void finish_frame(Worker &) = 0;

	friend qbrt_value * get_context(CodeFrame *, const std::string &);
	friend qbrt_value * add_context(CodeFrame *, const std::string &);

private:
	std::map< std::string, qbrt_value > frame_context;

public:
	typedef std::list< CodeFrame * > List;
};

struct FunctionCall
: public CodeFrame
{
	qbrt_value *result;
	qbrt_value *reg_data;
	const FunctionHeader *header;
	const Module *mod;
	int regc;

	FunctionCall(qbrt_value &result, function_value &func)
	: CodeFrame(CFT_CALL)
	, result(&result)
	, reg_data(func.reg)
	, header(func.func.header)
	, mod(func.func.mod)
	, regc(func.num_values())
	{}
	FunctionCall(CodeFrame &parent, qbrt_value &result, function_value &func)
	: CodeFrame(parent, CFT_CALL)
	, result(&result)
	, reg_data(func.reg)
	, header(func.func.header)
	, mod(func.func.mod)
	, regc(func.num_values())
	{}
	FunctionCall(function_value &func);

	virtual void finish_frame(Worker &);

	FunctionCall & function_call() { return *this; }
	const FunctionCall & function_call() const { return *this; }
	const char * name() const;

	uint8_t num_values() const { return regc; }
	qbrt_value & value(uint8_t i) { return reg_data[i]; }
	const qbrt_value & value(uint8_t i) const { return reg_data[i]; }
};

qbrt_value & CodeFrame::reg(uint8_t i)
{
	return function_call().reg_data[i];
}

static inline const instruction & frame_instruction(const CodeFrame &f)
{
	return *(const instruction *) (f.function_call().header->code() + f.pc);
}

struct ParallelPath
: public CodeFrame
{
	ParallelPath(CodeFrame &parent)
	: CodeFrame(parent, CFT_LOCAL_FORK)
	, f_call(parent.function_call())
	{}
	FunctionCall & function_call() { return f_call; }
	const FunctionCall & function_call() const { return f_call; }

	virtual void finish_frame(Worker &);

	uint8_t num_values() const { return f_call.regc; }
	qbrt_value & value(uint8_t i) { return f_call.reg_data[i]; }
	const qbrt_value & value(uint8_t i) const { return f_call.reg_data[i]; }

private:
	FunctionCall &f_call;
};

static inline ParallelPath * fork_frame(CodeFrame &src)
{
	ParallelPath *pp = new ParallelPath(src);
	src.fork.insert(pp);
	return pp;
}

struct ProcessRoot
{
	Worker *owner;
	FunctionCall *call;
	Pipe recv;
	qbrt_value result;
	uint64_t pid;

	ProcessRoot(uint64_t pid, FunctionCall *call)
	: owner(NULL)
	, call(call)
	, recv()
	, pid(pid)
	{}

	typedef std::map< uint64_t, ProcessRoot * > Map;
};


/**
 * Function call always assigned to the same worker
 *
 * Parallel path sometimes assigned to the same worker, sometimes a different
 * one. No problems when assigned to the same worker.
 *
 * When assigned to a different worker also a different thread.
 * Problems ensue on writable variables.
 * Use message passing to keep things unshared?
 * Normal locking on writable variables?
 * Static analysis to prohibit multiple use of writable vars?
 *
 * These problems can probably be punted on for now and worked out
 * once it's time to add multiple workers
 */
struct Worker
{
	Application &app;
	ModuleMap module;
	ProcessRoot::Map process;
	pthread_t thread;
	pthread_attr_t thread_attr;
	CodeFrame *current;
	CodeFrame::List *fresh;
	CodeFrame::List *stale;
	qbrt_value drain;
	int epfd;
	int iocount;
	WorkerID id;
	TaskID next_taskid;
	TaskID next_pid;

	Worker(Application &, WorkerID);

	bool empty() const;
};

void findtask(Worker &);
inline const Module * current_module(const Worker &w)
{
	return w.current->function_call().mod;
}
const Module * find_module(const Worker &, const std::string &modname);
const Module * load_module(Worker &, const std::string &modname);

void gotowork(Worker &);
void * launch_worker(void *);


struct Application
{
	typedef std::map< WorkerID, Worker * > WorkerMap;
	WorkerMap worker;
	ModuleMap module;
	ProcessRoot::Map newproc;
	ProcessRoot::Map recv;
	pthread_spinlock_t application_lock;
	WorkerID next_workerid;
	uint64_t pid_count;
	bool running;

	Application();
	~Application();

private:
	// don't use this. that doesn't make any sense at all.
	// it's only here to enforce the point.
	Application(const Application &);
};

const Module * find_app_module(Application &, const std::string &modname);
const Module * load_module(Application &, const std::string &modname);
void load_module(Application &, const Module *);
bool send_msg(Application &, uint64_t pid, const qbrt_value &src);
Worker & new_worker(Application &);
ProcessRoot * new_process(Application &, FunctionCall *);
void application_loop(Application &);

#endif

#include "qbrt/schedule.h"
#include "qbrt/module.h"

using namespace std;


qbrt_value * get_context(CodeFrame *f, const string &name)
{
	while (f) {
		if (f->context_data.find(name) == f->context_data.end()) {
			f = f->parent;
			continue;
		}
		return &f->context_data[name];
	}
	return NULL;
}

qbrt_value * add_context(CodeFrame *f, const string &name)
{
	qbrt_value *ctx = get_context(f, name);
	if (ctx) {
		return ctx;
	}
	return &f->context_data[name];
}

void CodeFrame::io_pop()
{
	if (!io) {
		return;
	}
	delete io;
	io = NULL;
}

const char * FunctionCall::name() const
{
	return fetch_string(mod->o->resource, resource->name_idx);
}

void FunctionCall::finish_frame(Worker &w)
{
	CodeFrame *call = w.current;
	w.current = w.current->parent;
	if (call->fork.empty()) {
		delete call;
	} else {
		w.stale->push_back(call);
	}
}

void ParallelPath::finish_frame(Worker &w)
{
	w.current->parent->fork.erase(w.current);
	if (w.current->fork.empty()) {
		delete w.current;
	} else {
		w.stale->push_back(w.current);
	}
	w.current = NULL;
	findtask(w);
}


Worker::Worker(Application &app, WorkerID id)
: app(app)
, module()
, current(NULL)
, fresh(new CodeFrame::List())
, stale(new CodeFrame::List())
, drain()
, epfd(0)
, iocount(0)
, id(id)
, next_taskid(0)
{
	epfd = epoll_create(1);
	if (epfd < 0) {
		perror("epoll_create failure");
	}
}

void findtask(Worker &w)
{
	if (w.fresh->empty()) {
		if (w.stale->empty()) {
			// all tasks are waiting on io
			return;
		}
		// out of fresh, swap fresh and stale
		CodeFrame::List *tmp = w.fresh;
		w.fresh = w.stale;
		w.stale = tmp;
	}
	// move the first fresh task to task
	w.current = w.fresh->front();
	w.fresh->pop_front();
}

const Module * find_module(const Worker &w, const std::string &modname)
{
	if (modname.empty()) {
		// if the module name is empty, return the current module
		return current_module(w);
	}

	std::map< std::string, Module * >::const_iterator it(w.module.begin());
	for (; it!=w.module.end(); ++it) {
		if (modname == it->first) {
			return it->second;
		}
	}
	return NULL;
}

Function find_override(Worker &w, const char * protocol_mod
		, const char *protocol_name, const Type &t
		, const char *funcname)
{
	std::map< std::string, Module * >::const_iterator it;
	it = w.module.begin();
	for (; it!=w.module.end(); ++it) {
		Function f(it->second->fetch_override(protocol_mod
					, protocol_name, t, funcname));
		if (f) {
			return f;
		}
	}
	return Function();
}

const ProtocolResource * find_function_protocol(Worker &w, const Function &f)
{
	if (f.resource->fcontext == PFC_NONE) {
		cerr << "No function protocol for no context\n";
		return NULL;
	}

	int fct(PFC_TYPE(f.resource->fcontext));
	const ResourceTable &res(f.obj->resource);
	const ProtocolResource *proto;
	uint16_t ctx(f.resource->context_idx);
	if (fct == FCT_POLYMORPH) {
		const PolymorphResource *poly;
		poly = res.ptr< PolymorphResource >(ctx);
		const ModSym *protoms = res.ptr< ModSym >(poly->protocol_idx);
		const char *proto_modname;
		const char *protosym;
		proto_modname = fetch_string(res, protoms->mod_name);
		protosym = fetch_string(res, protoms->sym_name);
		const Module *proto_mod(find_module(w, proto_modname));
		proto = proto_mod->fetch_protocol(protosym);
		return proto;
	} else if (fct == FCT_PROTOCOL) {
		return res.ptr< ProtocolResource >(ctx);
	}
	cerr << "where'd you find that function context? " << fct << endl;
	return NULL;
}

Function find_overridden_function(Worker &w, const Function &func)
{
	if (func.resource->fcontext != PFC_OVERRIDE) {
		return Function();
	}

	const ResourceTable &res(func.obj->resource);
	const ProtocolResource *proto;

	uint16_t polymorph_index(func.resource->context_idx);
	const PolymorphResource *poly;
	poly = res.ptr< PolymorphResource >(polymorph_index);

	const ModSym *protoms = res.ptr< ModSym >(poly->protocol_idx);
	const char *proto_modname;
	const char *protosym;
	proto_modname = fetch_string(res, protoms->mod_name);
	protosym = fetch_string(res, protoms->sym_name);

	const Module *mod(find_module(w, proto_modname));
	if (!mod) {
		return Function();
	}
	return mod->fetch_protocol_function(protosym, func.name());
}

const Module * load_module(Worker &w, const string &objname)
{
	Module *mod = load_module(objname);
	w.module[objname] = mod;
	return mod;
}

void load_module(Worker &w, const string &modname, Module *mod)
{
	w.module[modname] = mod;
}

Worker & new_worker(Application &app)
{
	Worker *w = new Worker(app, app.next_workerid++);
	app.worker[w->id] = w;
	return *w;
}

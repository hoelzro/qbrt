#ifndef QBRT_TYPE_H
#define QBRT_TYPE_H

#include "qbrt/core.h"
#include <cstdio>
#include <sys/epoll.h>
#include <list>


struct StructFieldResource
{
	uint16_t type_mod_id;
	uint16_t type_name_id;
	uint16_t field_name_id;

	static const uint32_t SIZE = 6;
};

struct StructResource
{
	uint16_t name_id;
	uint16_t field_count;
	StructFieldResource field[];
};

struct StructField
{
	Type* type;
	std::string name;
};

struct Type
{
	std::string module;
	std::string name;
	uint8_t id;
	std::vector< StructField > field;

	Type(uint8_t id);
	Type(const std::string &mod, const std::string &name, uint16_t flds);
};


struct StreamIO
{
	int fd;
	uint32_t events;
	StreamIO(int fd, uint32_t e)
	: fd(fd)
	, events(e)
	{}
	virtual ~StreamIO() {}
	virtual void handle() = 0;
};

struct StreamReadline
: public StreamIO
{
	FILE *file;
	qbrt_value &dst;

	StreamReadline(int fd, FILE *f, qbrt_value &dest)
	: StreamIO(fd, EPOLLIN)
	, file(f)
	, dst(dest)
	{}

	virtual void handle();
};

struct StreamWrite
: public StreamIO
{
	const std::string &src;

	StreamWrite(int fd, const std::string &src)
	: StreamIO(fd, EPOLLOUT)
	, src(src)
	{}

	virtual void handle();
};

struct Stream
{
	int fd;
	FILE *file;

	Stream(int fd, FILE *file)
	: fd(fd)
	, file(file)
	{}

	StreamIO * readline(qbrt_value &dst)
	{
		return new StreamReadline(fd, file, dst);
	}
	StreamIO * write(const std::string &src)
	{
		return new StreamWrite(fd, src);
	}
	size_t read(std::string &);
};

struct Promise
{
	std::list< TaskID > waiter;
	TaskID promiser;

	Promise(TaskID tid)
	: promiser(tid)
	{}
};

#endif

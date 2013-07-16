#ifndef QBRT_IO_H
#define QBRT_IO_H

#include "qbrt/core.h"
#include <sys/epoll.h>


struct StreamIO
{
	Stream *stream;
	uint32_t events;

	StreamIO(Stream *s, uint32_t e)
	: stream(s)
	, events(e)
	{}
	virtual ~StreamIO() {}
	virtual void handle() = 0;
};

struct StreamGetline
: public StreamIO
{
	qbrt_value &dst;

	StreamGetline(Stream *s, qbrt_value &dest)
	: StreamIO(s, EPOLLIN)
	, dst(dest)
	{}

	virtual void handle();
};

struct StreamWrite
: public StreamIO
{
	const std::string &src;

	StreamWrite(Stream *s, const std::string &src)
	: StreamIO(s, EPOLLOUT)
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

	virtual ~Stream() {}
	virtual StreamIO * getline(qbrt_value &dst) = 0;
	virtual StreamIO * write(const std::string &src) = 0;
};

struct ByteStream
: public Stream
{
	ByteStream(int fd, FILE *file)
	: Stream(fd, file)
	{}

	StreamIO * getline(qbrt_value &dst);
	StreamIO * write(const std::string &src);
};

struct FileStream
: public Stream
{
	FileStream(int fd, FILE *file)
	: Stream(fd, file)
	{}

	StreamIO * getline(qbrt_value &dst);
	StreamIO * write(const std::string &src);
};

#endif

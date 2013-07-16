#include "io.h"
#include <iostream>
#include <stdio.h>
#include <cstdlib>
#include <unistd.h>

using namespace std;


void StreamGetline::handle()
{
	char *line = NULL;
	size_t n(0);
	size_t len(getline(&line, &n, stream->file));
	// get rid of that lame newline
	// I will probably need to undue this later
	if (line[len-1] == '\n') {
		line[len-1] = '\0';
	}
	qbrt_value::str(dst, line);
	free(line);
}

void StreamWrite::handle()
{
	size_t result(write(stream->fd, src.c_str(), src.size()));
	if (result < 0) {
		cerr << "failure of some kind\n";
	}
}

StreamIO * ByteStream::getline(qbrt_value &dst)
{
	return new StreamGetline(this, dst);
}

StreamIO * ByteStream::write(const string &src)
{
	return new StreamWrite(this, src);
}

StreamIO * FileStream::getline(qbrt_value &dst)
{
	StreamGetline io(this, dst);
	io.handle();
	return NULL;
}

StreamIO * FileStream::write(const string &src)
{
	StreamWrite io(this, src);
	io.handle();
	return NULL;
}

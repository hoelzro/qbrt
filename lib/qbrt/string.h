#ifndef QBRT_STRING_H
#define QBRT_STRING_H


struct StringResource
{
	uint16_t bytes;
	const char value[];
};

struct HashTagResource
{
	uint16_t length;
	const char value[];
};

#endif

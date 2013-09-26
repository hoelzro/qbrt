#ifndef QBRT_INSTRUCTION_SCHEDULE_H
#define QBRT_INSTRUCTION_SCHEDULE_H

#include "qbrt/core.h"

#pragma pack(push, 1)

struct newproc_instruction
: public instruction
{
	uint16_t pid;
	uint16_t func;

	newproc_instruction(reg_t pid, reg_t func)
	: instruction(OP_NEWPROC)
	, pid(pid)
	, func(func)
	{}

	static const uint8_t SIZE = 5;
};

struct recv_instruction
: public instruction
{
	uint16_t dst;

	recv_instruction(reg_t dst)
	: instruction(OP_RECV)
	, dst(dst)
	{}

	static const uint8_t SIZE = 3;
};

#pragma pack(pop)

#endif

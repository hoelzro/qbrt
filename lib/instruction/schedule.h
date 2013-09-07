#ifndef QBRT_INSTRUCTION_SCHEDULE_H
#define QBRT_INSTRUCTION_SCHEDULE_H

#include "qbrt/core.h"


struct newproc_instruction
: public instruction
{
	uint64_t opcode_data : 8;
	uint64_t pid : 16;
	uint64_t func : 16;

	newproc_instruction(reg_t pid, reg_t func)
	: opcode_data(OP_NEWPROC)
	, pid(pid)
	, func(func)
	{}

	static const uint8_t SIZE = 5;
};

struct recv_instruction
: public instruction
{
	uint64_t opcode_data : 8;
	uint64_t dst : 16;
	uint64_t tube : 16;

	recv_instruction(reg_t dst, reg_t tube)
	: opcode_data(OP_RECV)
	, dst(dst)
	, tube(tube)
	{}

	static const uint8_t SIZE = 5;
};


#endif

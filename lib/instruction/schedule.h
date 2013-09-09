#ifndef QBRT_INSTRUCTION_SCHEDULE_H
#define QBRT_INSTRUCTION_SCHEDULE_H

#include "qbrt/core.h"

#pragma pack(push, 1)

struct newproc_instruction
: public instruction
{
	uint8_t opcode_data;
	uint16_t pid;
	uint16_t func;

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
	uint8_t opcode_data;
	uint16_t dst;
	uint16_t tube;

	recv_instruction(reg_t dst, reg_t tube)
	: opcode_data(OP_RECV)
	, dst(dst)
	, tube(tube)
	{}

	static const uint8_t SIZE = 5;
};

#pragma pack(pop)

#endif

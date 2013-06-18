#ifndef QBRT_INSTRUCTION_SCHEDULE_H
#define QBRT_INSTRUCTION_SCHEDULE_H

#include "qbrt/core.h"


struct newproc_instruction
: public instruction
{
	uint64_t opcode_data : 8;
	uint64_t pid : 16;
	uint64_t func : 16;
	uint64_t reserved : 24;

	newproc_instruction(reg_t pid, reg_t func)
	: opcode_data(OP_NEWPROC)
	, pid(pid)
	, func(func)
	, reserved(0)
	{}

	static const uint8_t SIZE = 5;
};


#endif

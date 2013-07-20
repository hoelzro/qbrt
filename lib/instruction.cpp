#include "instruction.h"
#include "qbrt/core.h"
#include "qbrt/arithmetic.h"
#include "qbrt/string.h"
#include "qbrt/function.h"
#include "qbrt/logic.h"
#include "qbrt/list.h"
#include "qbrt/tuple.h"
#include "instruction/schedule.h"
#include <iostream>

using namespace std;


template < typename I >
uint8_t iwriter(ostream &out, const I &i)
{
	out.write((char *) &i, I::SIZE);
	return I::SIZE;
}

#define DEFINE_IWRITER(i) \
	template uint8_t iwriter< i##_instruction >(ostream & \
			, const i##_instruction &);

DEFINE_IWRITER(binaryop);
DEFINE_IWRITER(consti);
DEFINE_IWRITER(consts);
DEFINE_IWRITER(consthash);
DEFINE_IWRITER(call);
DEFINE_IWRITER(fork);
DEFINE_IWRITER(lfunc);
DEFINE_IWRITER(lcontext);
DEFINE_IWRITER(loadtype);
DEFINE_IWRITER(loadobj);
DEFINE_IWRITER(lpfunc);
DEFINE_IWRITER(newproc);
DEFINE_IWRITER(recv);
DEFINE_IWRITER(stracc);
DEFINE_IWRITER(unimorph);
DEFINE_IWRITER(move);
DEFINE_IWRITER(ref);
DEFINE_IWRITER(copy);
DEFINE_IWRITER(return);
DEFINE_IWRITER(goto);
DEFINE_IWRITER(brbool);
DEFINE_IWRITER(brcmp);
DEFINE_IWRITER(breq);
DEFINE_IWRITER(brfail);
DEFINE_IWRITER(ctuple);
DEFINE_IWRITER(stuple);
DEFINE_IWRITER(clist);
DEFINE_IWRITER(cons);

instruction_writer WRITER[NUM_OP_CODES] = {0};

void init_writers()
{
	WRITER[OP_IADD] = (instruction_writer) iwriter<binaryop_instruction>;
	WRITER[OP_CALL] = (instruction_writer) iwriter<call_instruction>;
	WRITER[OP_RETURN] = (instruction_writer) iwriter<return_instruction>;
	WRITER[OP_CFAILURE] =
		(instruction_writer) iwriter<cfailure_instruction>;
	WRITER[OP_CONSTI] = (instruction_writer) iwriter<consti_instruction>;
	WRITER[OP_CONSTS] = (instruction_writer) iwriter<consts_instruction>;
	WRITER[OP_CONSTHASH] =
		(instruction_writer) iwriter<consthash_instruction>;
	WRITER[OP_LCONTEXT] = (instruction_writer)iwriter<lcontext_instruction>;
	WRITER[OP_FORK] = (instruction_writer) iwriter<fork_instruction>;
	WRITER[OP_IDIV] = (instruction_writer) iwriter<binaryop_instruction>;
	WRITER[OP_IMULT] = (instruction_writer) iwriter<binaryop_instruction>;
	WRITER[OP_ISUB] = (instruction_writer) iwriter<binaryop_instruction>;
	WRITER[OP_LFUNC] = (instruction_writer)iwriter<lfunc_instruction>;
	WRITER[OP_LOADTYPE] = (instruction_writer)iwriter<loadtype_instruction>;
	WRITER[OP_LOADOBJ] = (instruction_writer)iwriter<loadobj_instruction>;
	WRITER[OP_LPFUNC] = (instruction_writer)iwriter<lpfunc_instruction>;
	WRITER[OP_NEWPROC] = (instruction_writer)iwriter<newproc_instruction>;
	WRITER[OP_RECV] = (instruction_writer)iwriter<recv_instruction>;
	WRITER[OP_STRACC] = (instruction_writer)iwriter<stracc_instruction>;
	WRITER[OP_UNIMORPH] = (instruction_writer)iwriter<unimorph_instruction>;
	WRITER[OP_MOVE] = (instruction_writer) iwriter<move_instruction>;
	WRITER[OP_REF] = (instruction_writer) iwriter<ref_instruction>;
	WRITER[OP_COPY] = (instruction_writer) iwriter<copy_instruction>;
	WRITER[OP_GOTO] = (instruction_writer) iwriter<goto_instruction>;
	WRITER[OP_BRF] = (instruction_writer) iwriter<brbool_instruction>;
	WRITER[OP_BRT] = (instruction_writer) iwriter<brbool_instruction>;
	WRITER[OP_BRNE] = (instruction_writer) iwriter<brcmp_instruction>;
	WRITER[OP_BREQ] = (instruction_writer) iwriter<breq_instruction>;
	WRITER[OP_BRFAIL] = (instruction_writer) iwriter<brfail_instruction>;
	WRITER[OP_BRNFAIL] = (instruction_writer) iwriter<brfail_instruction>;
	WRITER[OP_CTUPLE] = (instruction_writer) iwriter<ctuple_instruction>;
	WRITER[OP_STUPLE] = (instruction_writer) iwriter<stuple_instruction>;
	WRITER[OP_CLIST] = (instruction_writer) iwriter<clist_instruction>;
	WRITER[OP_CONS] = (instruction_writer) iwriter<cons_instruction>;
	WRITER[OP_WAIT] = (instruction_writer) iwriter<wait_instruction>;
}

uint8_t write_instruction(ostream &out, const instruction &i)
{
	instruction_writer w = WRITER[i.opcode()];
	if (!w) {
		cerr << "instruction writer is null for: " << (int) i.opcode()
			<< endl;
	}
	return w(out, i);
}
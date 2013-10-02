#include "instruction.h"
#include "qbrt/core.h"
#include "qbrt/tuple.h"
#include "instruction/arithmetic.h"
#include "instruction/function.h"
#include "instruction/logic.h"
#include "instruction/schedule.h"
#include "instruction/string.h"
#include "instruction/type.h"
#include <iostream>
#include <cstdlib>

using namespace std;


instruction_writer WRITER[NUM_OP_CODES] = {0};
static uint8_t INSTRUCTION_SIZE[NUM_OP_CODES];


template < typename I >
uint8_t iwriter(ostream &out, const I &i)
{
	out.write((char *) &i, I::SIZE);
	return I::SIZE;
}

#define DEFINE_IWRITER(i) \
	template uint8_t iwriter< i##_instruction >(ostream & \
			, const i##_instruction &);

void init_instruction_sizes()
{
	INSTRUCTION_SIZE[OP_CALL] = call_instruction::SIZE;
	INSTRUCTION_SIZE[OP_CALL1] = call1_instruction::SIZE;
	INSTRUCTION_SIZE[OP_CALL2] = call2_instruction::SIZE;
	INSTRUCTION_SIZE[OP_RETURN] = return_instruction::SIZE;
	INSTRUCTION_SIZE[OP_CFAILURE] = cfailure_instruction::SIZE;
	INSTRUCTION_SIZE[OP_CMP_EQ] = cmp_instruction::SIZE;
	INSTRUCTION_SIZE[OP_CMP_NOTEQ] = cmp_instruction::SIZE;
	INSTRUCTION_SIZE[OP_CMP_GT] = cmp_instruction::SIZE;
	INSTRUCTION_SIZE[OP_CMP_GTEQ] = cmp_instruction::SIZE;
	INSTRUCTION_SIZE[OP_CMP_LT] = cmp_instruction::SIZE;
	INSTRUCTION_SIZE[OP_CMP_LTEQ] = cmp_instruction::SIZE;
	INSTRUCTION_SIZE[OP_CONSTI] = consti_instruction::SIZE;
	INSTRUCTION_SIZE[OP_CONSTS] = consts_instruction::SIZE;
	INSTRUCTION_SIZE[OP_CONSTHASH] = consthash_instruction::SIZE;
	INSTRUCTION_SIZE[OP_FORK] = fork_instruction::SIZE;
	INSTRUCTION_SIZE[OP_IADD] = binaryop_instruction::SIZE;
	INSTRUCTION_SIZE[OP_IDIV] = binaryop_instruction::SIZE;
	INSTRUCTION_SIZE[OP_IMULT] = binaryop_instruction::SIZE;
	INSTRUCTION_SIZE[OP_ISUB] = binaryop_instruction::SIZE;
	INSTRUCTION_SIZE[OP_LCONTEXT] = lcontext_instruction::SIZE;
	INSTRUCTION_SIZE[OP_LCONSTRUCT] = lconstruct_instruction::SIZE;
	INSTRUCTION_SIZE[OP_LFUNC] = lfunc_instruction::SIZE;
	INSTRUCTION_SIZE[OP_LOADTYPE] = loadtype_instruction::SIZE;
	INSTRUCTION_SIZE[OP_LOADOBJ] = loadobj_instruction::SIZE;
	INSTRUCTION_SIZE[OP_MATCH] = match_instruction::SIZE;
	INSTRUCTION_SIZE[OP_MATCHARGS] = matchargs_instruction::SIZE;
	INSTRUCTION_SIZE[OP_MOVE] = move_instruction::SIZE;
	INSTRUCTION_SIZE[OP_REF] = ref_instruction::SIZE;
	INSTRUCTION_SIZE[OP_COPY] = copy_instruction::SIZE;
	INSTRUCTION_SIZE[OP_FIELDGET] = fieldget_instruction::SIZE;
	INSTRUCTION_SIZE[OP_FIELDSET] = fieldset_instruction::SIZE;
	INSTRUCTION_SIZE[OP_GOTO] = goto_instruction::SIZE;
	INSTRUCTION_SIZE[OP_IF] = if_instruction::SIZE;
	INSTRUCTION_SIZE[OP_IFNOT] = if_instruction::SIZE;
	INSTRUCTION_SIZE[OP_IFFAIL] = iffail_instruction::SIZE;
	INSTRUCTION_SIZE[OP_IFNOTFAIL] = iffail_instruction::SIZE;
	INSTRUCTION_SIZE[OP_CTUPLE] = ctuple_instruction::SIZE;
	INSTRUCTION_SIZE[OP_STUPLE] = stuple_instruction::SIZE;
	INSTRUCTION_SIZE[OP_CLIST] = clist_instruction::SIZE;
	INSTRUCTION_SIZE[OP_CONS] = cons_instruction::SIZE;
	INSTRUCTION_SIZE[OP_NEWPROC] = newproc_instruction::SIZE;
	INSTRUCTION_SIZE[OP_PATTERNVAR] = patternvar_instruction::SIZE;
	INSTRUCTION_SIZE[OP_RECV] = recv_instruction::SIZE;
	INSTRUCTION_SIZE[OP_STRACC] = stracc_instruction::SIZE;
	INSTRUCTION_SIZE[OP_WAIT] = wait_instruction::SIZE;
}

uint8_t isize(uint8_t opcode)
{
	uint8_t sz(INSTRUCTION_SIZE[opcode]);
	if (sz == 0) {
		std::cerr << "Unset instruction size for opcode: "
			<< (int) opcode << std::endl;
		exit(1);
	}
	return sz;
}

DEFINE_IWRITER(binaryop);
DEFINE_IWRITER(consti);
DEFINE_IWRITER(consts);
DEFINE_IWRITER(consthash);
DEFINE_IWRITER(call);
DEFINE_IWRITER(call1);
DEFINE_IWRITER(call2);
DEFINE_IWRITER(fork);
DEFINE_IWRITER(fieldget);
DEFINE_IWRITER(fieldset);
DEFINE_IWRITER(lcontext);
DEFINE_IWRITER(lconstruct);
DEFINE_IWRITER(lfunc);
DEFINE_IWRITER(loadtype);
DEFINE_IWRITER(loadobj);
DEFINE_IWRITER(match);
DEFINE_IWRITER(matchargs);
DEFINE_IWRITER(newproc);
DEFINE_IWRITER(patternvar);
DEFINE_IWRITER(recv);
DEFINE_IWRITER(stracc);
DEFINE_IWRITER(move);
DEFINE_IWRITER(ref);
DEFINE_IWRITER(copy);
DEFINE_IWRITER(return);
DEFINE_IWRITER(goto);
DEFINE_IWRITER(if);
DEFINE_IWRITER(iffail);
DEFINE_IWRITER(ctuple);
DEFINE_IWRITER(stuple);
DEFINE_IWRITER(clist);
DEFINE_IWRITER(cons);

void init_writers()
{
	WRITER[OP_IADD] = (instruction_writer) iwriter<binaryop_instruction>;
	WRITER[OP_CALL] = (instruction_writer) iwriter<call_instruction>;
	WRITER[OP_CALL1] = (instruction_writer) iwriter<call1_instruction>;
	WRITER[OP_CALL2] = (instruction_writer) iwriter<call2_instruction>;
	WRITER[OP_RETURN] = (instruction_writer) iwriter<return_instruction>;
	WRITER[OP_CFAILURE] =
		(instruction_writer) iwriter<cfailure_instruction>;
	WRITER[OP_CMP_EQ] = (instruction_writer)iwriter<cmp_instruction>;
	WRITER[OP_CMP_NOTEQ] = (instruction_writer)iwriter<cmp_instruction>;
	WRITER[OP_CMP_GT] = (instruction_writer)iwriter<cmp_instruction>;
	WRITER[OP_CMP_GTEQ] = (instruction_writer)iwriter<cmp_instruction>;
	WRITER[OP_CMP_LT] = (instruction_writer)iwriter<cmp_instruction>;
	WRITER[OP_CMP_LTEQ] = (instruction_writer)iwriter<cmp_instruction>;
	WRITER[OP_CONSTI] = (instruction_writer) iwriter<consti_instruction>;
	WRITER[OP_CONSTS] = (instruction_writer) iwriter<consts_instruction>;
	WRITER[OP_CONSTHASH] =
		(instruction_writer) iwriter<consthash_instruction>;
	WRITER[OP_LCONTEXT] = (instruction_writer)iwriter<lcontext_instruction>;
	WRITER[OP_LCONSTRUCT] =
		(instruction_writer)iwriter<lconstruct_instruction>;
	WRITER[OP_FIELDGET] = (instruction_writer)iwriter<fieldget_instruction>;
	WRITER[OP_FIELDSET] = (instruction_writer)iwriter<fieldset_instruction>;
	WRITER[OP_FORK] = (instruction_writer) iwriter<fork_instruction>;
	WRITER[OP_IDIV] = (instruction_writer) iwriter<binaryop_instruction>;
	WRITER[OP_IMULT] = (instruction_writer) iwriter<binaryop_instruction>;
	WRITER[OP_ISUB] = (instruction_writer) iwriter<binaryop_instruction>;
	WRITER[OP_LFUNC] = (instruction_writer)iwriter<lfunc_instruction>;
	WRITER[OP_LOADTYPE] = (instruction_writer)iwriter<loadtype_instruction>;
	WRITER[OP_LOADOBJ] = (instruction_writer)iwriter<loadobj_instruction>;
	WRITER[OP_MATCH] = (instruction_writer)iwriter<match_instruction>;
	WRITER[OP_MATCHARGS] =
		(instruction_writer)iwriter<matchargs_instruction>;
	WRITER[OP_NEWPROC] = (instruction_writer)iwriter<newproc_instruction>;
	WRITER[OP_PATTERNVAR] =
		(instruction_writer)iwriter<patternvar_instruction>;
	WRITER[OP_RECV] = (instruction_writer)iwriter<recv_instruction>;
	WRITER[OP_STRACC] = (instruction_writer)iwriter<stracc_instruction>;
	WRITER[OP_MOVE] = (instruction_writer) iwriter<move_instruction>;
	WRITER[OP_REF] = (instruction_writer) iwriter<ref_instruction>;
	WRITER[OP_COPY] = (instruction_writer) iwriter<copy_instruction>;
	WRITER[OP_GOTO] = (instruction_writer) iwriter<goto_instruction>;
	WRITER[OP_IF] = (instruction_writer) iwriter<if_instruction>;
	WRITER[OP_IFNOT] = (instruction_writer) iwriter<if_instruction>;
	WRITER[OP_IFFAIL] = (instruction_writer) iwriter<iffail_instruction>;
	WRITER[OP_IFNOTFAIL] = (instruction_writer) iwriter<iffail_instruction>;
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
			& DIE;
	}
	w(out, i);
	return isize(i.opcode());
}

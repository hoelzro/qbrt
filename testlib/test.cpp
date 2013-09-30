#include "instruction/logic.h"
#include "instruction/schedule.h"
#include "instruction/type.h"
#include "qbrt/function.h"
#include "accertion.h"


CCTEST(check_function_instruction_sizes)
{
	accert(sizeof(call_instruction)) == call_instruction::SIZE;
	accert(sizeof(return_instruction)) == return_instruction::SIZE;
	accert(sizeof(cfailure_instruction)) == cfailure_instruction::SIZE;
	accert(sizeof(lcontext_instruction)) == lcontext_instruction::SIZE;
	accert(sizeof(lfunc_instruction)) == lfunc_instruction::SIZE;
	accert(sizeof(loadtype_instruction)) == loadtype_instruction::SIZE;
	accert(sizeof(loadobj_instruction)) == loadobj_instruction::SIZE;
	accert(sizeof(move_instruction)) == move_instruction::SIZE;
	accert(sizeof(ref_instruction)) == ref_instruction::SIZE;
	accert(sizeof(copy_instruction)) == copy_instruction::SIZE;
}

CCTEST(check_logic_instruction_sizes)
{
	accert(sizeof(goto_instruction)) == goto_instruction::SIZE;
	accert(sizeof(if_instruction)) == if_instruction::SIZE;
	accert(sizeof(cmp_instruction)) == cmp_instruction::SIZE;
	accert(sizeof(iffail_instruction)) == iffail_instruction::SIZE;
	accert(sizeof(match_instruction)) == match_instruction::SIZE;
	accert(sizeof(matchargs_instruction)) == matchargs_instruction::SIZE;
	accert(sizeof(fork_instruction)) == fork_instruction::SIZE;
	accert(sizeof(wait_instruction)) == wait_instruction::SIZE;
}

CCTEST(check_schedule_instruction_sizes)
{
	accert(sizeof(newproc_instruction)) == newproc_instruction::SIZE;
	accert(sizeof(recv_instruction)) == recv_instruction::SIZE;
}

CCTEST(check_type_instruction_sizes)
{
	accert(sizeof(clist_instruction)) == clist_instruction::SIZE;
	accert(sizeof(cons_instruction)) == cons_instruction::SIZE;
	accert(sizeof(ctuple_instruction)) == ctuple_instruction::SIZE;
	accert(sizeof(fieldget_instruction)) == fieldget_instruction::SIZE;
	accert(sizeof(fieldset_instruction)) == fieldset_instruction::SIZE;
	accert(sizeof(lconstruct_instruction)) == lconstruct_instruction::SIZE;
	accert(sizeof(patternvar_instruction)) == patternvar_instruction::SIZE;
}


int main(int argc, const char **argv) { return accertion_main(argc, argv); }

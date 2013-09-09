#include "accertion.h"


CCTEST(instruction_size)
{
	accert(5) == 5;
}


int main(int argc, const char **argv)
{
	return accertion_main(argc, argv);
}

#ifndef QBRT_INSTRUCTION_H
#define QBRT_INSTRUCTION_H

#include "qbrt/core.h"

typedef uint8_t (*instruction_writer)(std::ostream &, const instruction &);

extern instruction_writer WRITER[NUM_OP_CODES];

void init_writers();
uint8_t write_instruction(std::ostream &, const instruction &);

#endif

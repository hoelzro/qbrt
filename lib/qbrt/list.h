#ifndef QBRT_LIST_H
#define QBRT_LIST_H

#include "qbrt/core.h"

struct List
{
	qbrt_value value;
	List *next;

	List()
		: next(0)
	{}

	List(const qbrt_value &val, List *head)
		: value(val)
		, next(head)
	{}
};

static inline void cons(const qbrt_value &val, List *&head)
{
	List *newhead = new List(val, head);
	head = newhead;
}

static inline qbrt_value head(List *head)
{
	if (!head) {
		return qbrt_value();
	}
	return head->value;
}

static inline List * pop(List *head)
{
	return head->next;
}

static inline bool empty(const List *l)
{
	return !l;
}

static inline List * reverse(List *head)
{
	List *next;
	List *last = NULL;
	while (head) {
		next = head->next;
		head->next = last;
		last = head;
		head = next;
	}
	return last;
}

struct clist_instruction
: public instruction
{
	uint32_t opcode_data : 8;
	uint32_t dst : 16;

	clist_instruction(reg_t dst)
		: opcode_data(OP_CLIST)
		, dst(dst)
	{}

	static const uint8_t SIZE = 3;
};

struct cons_instruction
: public instruction
{
	uint32_t opcode_data : 8;
	uint32_t head : 16;
	uint32_t item : 16;

	cons_instruction(reg_t head, reg_t item)
		: opcode_data(OP_CLIST)
		, head(head)
		, item(item)
	{}

	static const uint8_t SIZE = 5;
};

#endif

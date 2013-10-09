#ifndef QBRT_VECTOR_H
#define QBRT_VECTOR_H

#include "qbrt/core.h"
#include <string.h>

struct Vector
{
	qbrt_value value[16];
	Vector *higher[16];

	Vector()
		: value()
	{
		memset(higher, 0, sizeof(higher));
	}

	Vector(const Vector &v)
	{
		memcpy(value, v.value, sizeof(value));
		memcpy(higher, v.higher, sizeof(higher));
	}
};

Vector * set(Vector *v, uint32_t i, const qbrt_value &val)
{
	uint32_t lower(i & 0xf);
	uint32_t upper(i >> 4);
	Vector *copy;
	if (v) {
		copy = new Vector(*v);
	} else {
		copy = new Vector();
	}
	if (upper == 0) {
		copy->value[lower] = val;
	} else {
		copy->higher[lower] = set(v->higher[lower], upper, val);
	}
	return copy;
}

qbrt_value * get(Vector *v, uint32_t i)
{
	if (!v) {
		return NULL;
	}
	uint32_t lower(i & 0xf);
	uint32_t upper(i >> 4);
	if (upper == 0) {
		return &v->value[lower];
	} else {
		return get(v->higher[lower], upper);
	}
}

#endif

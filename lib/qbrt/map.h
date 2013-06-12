#ifndef QBRT_MAP_H
#define QBRT_MAP_H

#include "core.h"
#include <string.h>

struct Map
{
	qbrt_value key;
	qbrt_value value;
	Map *left;
	Map *right;

	Map()
		: value()
		, left(NULL)
		, right(NULL)
	{}

	Map(const qbrt_value &key, const qbrt_value &val)
		: key(key)
		, value(val)
		, left(NULL)
		, right(NULL)
	{}
};


Map * insert(Map *m, const qbrt_value &key, const qbrt_value &val)
{
	if (!m) {
		return new Map(key, val);
	}
	Map *copy = new Map(*m);
	int comparison(qbrt_compare(key, m->key));
	if (comparison < 0) {
		copy->left = insert(copy->left, key, val);
	} else if (comparison > 0) {
		copy->right = insert(copy->right, key, val);
	} else {
		copy->value = val;
	}
	return copy;
}

qbrt_value * find(Map *m, const qbrt_value &key)
{
	if (!m) {
		return NULL;
	}
	int comparison(qbrt_compare(key, m->key));
	if (comparison < 0) {
		return find(m->left, key);
	} else if (comparison > 0) {
		return find(m->right, key);
	}
	return &m->value;
}

#endif

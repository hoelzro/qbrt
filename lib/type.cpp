#include "qbrt/type.h"
#include "qbrt/module.h"
#include <string.h>

using namespace std;


const DataTypeResource * Construct::datatype() const
{
	return mod.resource.ptr< DataTypeResource >(resource.datatype_idx);
}

const char * Construct::name() const
{
	return fetch_string(mod.resource, resource.name_idx);
}

bool Construct::compare(const Construct &a, const Construct &b)
{
	if (a.mod.name < b.mod.name) {
		return -1;
	}
	if (a.mod.name > b.mod.name) {
		return +1;
	}
	const char *aname = a.name();
	const char *bname = b.name();
	return strcmp(aname, bname);
}

void load_construct_value_types(ostringstream &out, const Construct &c)
{
	bool first(true);
	const DataTypeResource *dtr = c.datatype();
	const char *datatype_name = fetch_string(c.mod.resource, dtr->name_idx);
	const ResourceTable &res(c.mod.resource);
	out << c.mod.name << '/' << datatype_name;
	if (dtr->argc == 0) {
		return;
	}
	if (dtr->argc > 1) {
		out << "(too many arguments)";
	}
	out << '(';
	bool found(false);
	for (int i(0); i<c.resource.fld_count; ++i) {
		if (first) {
			first = false;
		} else {
			out << ',';
		}
		const ParamResource &param(c.resource.fields[i]);
		const TypeSpecResource *tsr;
		tsr = res.ptr< TypeSpecResource >(param.type_idx);
		const ModSym &typems = fetch_modsym(res, tsr->name_idx);
		const char *modnam = fetch_string(res, typems.mod_name);
		const char *typnam = fetch_string(res, typems.sym_name);

		if (modnam[0] == '*' && modnam[1] == '\0') {
			qbrt_value::append_type(out, c.value(i));
			found = true;
			break;
		}
	}
	if (!found) {
		cerr << "(what type is this?)";
	}
	out << ')';
}


void List::push(qbrt_value &head, const qbrt_value &item)
{
	qbrt_value tmp(head);
	Module::load_construct(head, head.data.cons->mod, "Node");
	head.data.cons->value(0) = item;
	head.data.cons->value(1) = tmp;
}

void List::head(qbrt_value &result, const qbrt_value &head)
{
	if (head.type->id != VT_LIST) {
		cerr <<"head arg not a list: "<< (int) head.type->id << endl;
		// set failure in result
		return;
	}
	result = head.data.cons->value(0);
}

void List::is_empty(qbrt_value &result, const qbrt_value &head)
{
	if (head.type->id != VT_LIST) {
		qbrt_value::fail(result, FAIL_TYPE("list/is_empty", 0));
		return;
	}
	bool empty(strcmp(head.data.cons->name(), "Empty") == 0);
	qbrt_value::b(result, empty);
}

void List::pop(qbrt_value &result, const qbrt_value &head)
{
	if (head.type->id != VT_LIST) {
		qbrt_value::fail(result, FAIL_TYPE("list/pop", 0));
		cerr <<"head arg not a construct: "<< (int)head.type->id<< endl;
		// set failure in result
		return;
	}
	result = head.data.cons->value(1);
}

void List::reverse(qbrt_value &result, const qbrt_value &head)
{
	Module::load_construct(result, head.data.cons->mod, "Empty");
	qbrt_value next(head);
	qbrt_value check;
	qbrt_value item;
	List::is_empty(check, next);
	while (!check.data.b) {
		List::head(item, next);
		List::push(result, item);
		List::pop(next, next);
		List::is_empty(check, next);
	}
}

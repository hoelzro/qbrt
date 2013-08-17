#include "qbrt/type.h"
#include "qbrt/module.h"

using namespace std;


const DataTypeResource * Construct::datatype() const
{
	return mod.resource.ptr< DataTypeResource >(resource.datatype_idx);
}

void load_construct_value_types(ostringstream &out, const Construct &c)
{
	bool first(true);
	const DataTypeResource *dtr = c.datatype();
	const char *datatype_name = fetch_string(c.mod.resource, dtr->name_idx);
	out << c.mod.name << '/' << datatype_name << '(';
	for (int i(0); i<c.resource.fld_count; ++i) {
		if (first) {
			first = false;
		} else {
			out << ',';
		}
		qbrt_value::append_type(out, c.value(i));
	}
	out << ')';
}


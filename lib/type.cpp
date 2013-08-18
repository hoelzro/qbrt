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

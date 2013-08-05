#include "qbrt/function.h"
#include "qbrt/resourcetype.h"
#include "qbrt/module.h"
#include <cstdlib>

using namespace std;


const char * QbrtFunction::name() const
{
	return fetch_string(mod->resource, header->name_idx);
}

uint32_t QbrtFunction::code_offset() const
{
	return code - mod->resource.data
		+ ResourceTable::DATA_OFFSET;
}

const char * QbrtFunction::protocol_name() const
{
	uint16_t name_idx(0);
	switch (PFC_TYPE(header->fcontext)) {
		case FCT_TRADITIONAL:
			return NULL;
		case FCT_PROTOCOL: {
			const ProtocolResource *proto;
			proto = mod->resource.ptr< const ProtocolResource >(
					header->context_idx);
			name_idx = proto->name_idx;
			break; }
		case FCT_POLYMORPH: {
			const PolymorphResource *poly;
			poly = mod->resource.ptr< const PolymorphResource >(
					header->context_idx);
			const ModSym *protoms;
			protoms = mod->resource.ptr< const ModSym >(
					poly->protocol_idx);
			name_idx = protoms->sym_name;
			break; }
	}
	return fetch_string(mod->resource, name_idx);
}

const char * QbrtFunction::protocol_module() const
{
	uint16_t mod_idx(0);
	switch (PFC_TYPE(header->fcontext)) {
		case FCT_TRADITIONAL:
			return NULL;
		case FCT_PROTOCOL:
			return mod->name.c_str();
		case FCT_POLYMORPH: {
			const PolymorphResource *poly;
			poly = mod->resource.ptr< const PolymorphResource >(
					header->context_idx);
			const ModSym *protoms;
			protoms = mod->resource.ptr< const ModSym >(
					poly->protocol_idx);
			mod_idx = protoms->mod_name;
			break; }
	}
	return fetch_string(mod->resource, mod_idx);
}

const char * CFunction::protocol_module() const
{
	return proto_module.empty() ? NULL : proto_module.c_str();
}

const char * CFunction::protocol_name() const
{
	return proto_name.empty() ? NULL : proto_name.c_str();
}


function_value::function_value(const Function *f)
: func(f)
, argc(f->argc())
, regc(f->regtotal())
{
	regv = (qbrt_value *) malloc(regc * sizeof(qbrt_value));
	new (regv) qbrt_value[regc];
}

void function_value::realloc(uint8_t new_regc)
{
	size_t newsize(new_regc * sizeof(qbrt_value));
	qbrt_value *newreg =
		(qbrt_value *) ::realloc(this->regv, newsize);
	this->regv = newreg;
	for (int i(this->regc); i<new_regc; ++i) {
		new (&this->regv[i]) qbrt_value();
	}
}

void load_function_param_types(string &paramstr, const function_value &func)
{
	paramstr = "";
	for (int i(0); i<func.argc; ++i) {
		const qbrt_value &val(func.value(i));
		const Type *typ(val.type);
		paramstr += typ->module;
		paramstr += '/';
		paramstr += typ->name;
		paramstr += ';';
	}
}

void reassign_func(function_value &funcval, const Function *newfunc)
{
	int new_regc(newfunc->regtotal());
	if (funcval.regc < new_regc) {
		funcval.realloc(new_regc);
	}
	funcval.func = newfunc;
}

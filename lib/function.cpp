#include "qbrt/function.h"
#include "qbrt/resourcetype.h"
#include "qbrt/module.h"


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

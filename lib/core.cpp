#include "qbrt/core.h"
#include "qbrt/type.h"
#include <iostream>
#include <sstream>
#include "qbrt/function.h"
#include "qbrt/logic.h"
#include "qbrt/tuple.h"
#include "instruction/arithmetic.h"
#include "instruction/schedule.h"
#include "instruction/string.h"
#include "instruction/type.h"
#include <cstdlib>

using namespace std;

static string PRIMITIVE_MODULE[256];
static string PRIMITIVE_NAME[256];
uint8_t INSTRUCTION_SIZE[NUM_OP_CODES];
Death DIE;

static void init_primitive_modules()
{
	PRIMITIVE_MODULE[VT_VOID] = "core";
	PRIMITIVE_MODULE[VT_INT] = "core";
	PRIMITIVE_MODULE[VT_BOOL] = "core";
	PRIMITIVE_MODULE[VT_FLOAT] = "core";
	PRIMITIVE_MODULE[VT_FUNCTION] = "core";
	PRIMITIVE_MODULE[VT_STRING] = "core";
	PRIMITIVE_MODULE[VT_HASHTAG] = "core";
	PRIMITIVE_MODULE[VT_PATTERNVAR] = "core";
	PRIMITIVE_MODULE[VT_REF] = "core";
	PRIMITIVE_MODULE[VT_TUPLE] = "Tuple";
	PRIMITIVE_MODULE[VT_LIST] = "list";
	PRIMITIVE_MODULE[VT_MAP] = "Map";
	PRIMITIVE_MODULE[VT_VECTOR] = "Vector";
	PRIMITIVE_MODULE[VT_STREAM] = "io";
	PRIMITIVE_MODULE[VT_PROMISE] = "core";
	PRIMITIVE_MODULE[VT_KIND] = "core";
	PRIMITIVE_MODULE[VT_FAILURE] = "core";
}

static void init_primitive_names()
{
	PRIMITIVE_NAME[VT_VOID] = "Void";
	PRIMITIVE_NAME[VT_INT] = "Int";
	PRIMITIVE_NAME[VT_BOOL] = "Bool";
	PRIMITIVE_NAME[VT_FLOAT] = "Float";
	PRIMITIVE_NAME[VT_FUNCTION] = "Function";
	PRIMITIVE_NAME[VT_STRING] = "String";
	PRIMITIVE_NAME[VT_HASHTAG] = "HashTag";
	PRIMITIVE_NAME[VT_PATTERNVAR] = "PatternVar";
	PRIMITIVE_NAME[VT_REF] = "Ref";
	PRIMITIVE_NAME[VT_TUPLE] = "Tuple";
	PRIMITIVE_NAME[VT_LIST] = "List";
	PRIMITIVE_NAME[VT_MAP] = "Map";
	PRIMITIVE_NAME[VT_VECTOR] = "Vector";
	PRIMITIVE_NAME[VT_STREAM] = "Stream";
	PRIMITIVE_NAME[VT_PROMISE] = "Promise";
	PRIMITIVE_NAME[VT_KIND] = "Kind";
	PRIMITIVE_NAME[VT_FAILURE] = "Failure";
}

static const string & get_primitive_module(uint8_t id)
{
	static bool initialized(false);
	if (!initialized) {
		init_primitive_modules();
		initialized = true;
	}
	const string &mod(PRIMITIVE_MODULE[id]);
	if (mod.empty()) {
		cerr << "empty module for primitive type: " << (int) id << endl;
	}
	return mod;
}

static const std::string & get_primitive_name(uint8_t id)
{
	static bool initialized(false);
	if (!initialized) {
		init_primitive_names();
		initialized = true;
	}
	const string &name(PRIMITIVE_NAME[id]);
	if (name.empty()) {
		cerr << "empty name for primitive type: " << (int) id << endl;
	}
	return name;
}

static const uint8_t get_type_id(const string &mod, const string &name)
{
	for (uint16_t id(0); id<=0xff; ++id) {
		if (PRIMITIVE_MODULE[id] == mod && PRIMITIVE_NAME[id] == name) {
			return id;
		}
	}
	return VT_CONSTRUCT;
}


Type::Type(uint8_t id)
: module(get_primitive_module(id))
, name(get_primitive_name(id))
, id(id)
, argc(0)
{}

Type::Type(const string &mod, const string &name, uint8_t argc)
: module(mod)
, name(name)
, id(get_type_id(mod, name))
, argc(argc)
{}

Type TYPE_VOID(VT_VOID);
Type TYPE_KIND(VT_KIND);
Type TYPE_INT(VT_INT);
Type TYPE_STRING(VT_STRING);
Type TYPE_BOOL(VT_BOOL);
Type TYPE_FLOAT(VT_FLOAT);
Type TYPE_HASHTAG(VT_HASHTAG);
Type TYPE_REF(VT_REF);
Type TYPE_TUPLE(VT_TUPLE);
Type TYPE_FUNCTION(VT_FUNCTION);
Type TYPE_LIST(VT_LIST);
Type TYPE_MAP(VT_MAP);
Type TYPE_VECTOR(VT_VECTOR);
Type TYPE_STREAM(VT_STREAM);
Type TYPE_PATTERNVAR(VT_PATTERNVAR);
Type TYPE_PROMISE(VT_PROMISE);
Type TYPE_FAILURE(VT_FAILURE);


void qbrt_value::default_value(qbrt_value &v, const Type &t)
{
	switch (t.id) {
		case VT_INT:
			i(v, 0);
			break;
		case VT_STRING:
			str(v, "");
			break;
		case VT_BOOL:
			b(v, false);
			break;
		default:
			cerr << "No default value for: " << (int) t.id << endl;
			exit(1);
	}
}

void qbrt_value::set_void(qbrt_value &v)
{
	v.data.str = NULL;
	v.type = &TYPE_VOID;
}

void qbrt_value::ref(qbrt_value &v, qbrt_value &ref)
{
	if (&v == &ref) {
		cerr << "set self ref\n";
		return;
	}
	v.type = &TYPE_REF;
	v.data.ref = &ref;
}

void qbrt_value::copy(qbrt_value &dst, const qbrt_value &src)
{
	switch (src.type->id) {
		case VT_VOID:
			set_void(dst);
			break;
		case VT_INT:
			qbrt_value::i(dst, src.data.i);
			break;
		case VT_BOOL:
			qbrt_value::b(dst, src.data.b);
			break;
		case VT_FLOAT:
			qbrt_value::f(dst, src.data.f);
			break;
		case VT_STRING:
			qbrt_value::str(dst, *src.data.str);
			break;
		case VT_CONSTRUCT:
			qbrt_value::construct(dst, src.type, src.data.cons);
			break;
		default:
			cerr << "wtf you can't copy that!\n";
			break;
	}
}

bool qbrt_value::is_value_index(const qbrt_value &val)
{
	if (!val.data.reg) {
		return false;
	}
	return dynamic_cast< qbrt_value_index * >(val.data.reg);
}

bool qbrt_value::failed(const qbrt_value &val)
{
	return val.type->id == VT_FAILURE;
}

int16_t qbrt_value::get_field_index(const string &fldname) const
{
	switch (type->id) {
		case VT_FAILURE:
			// Yes, this is ugly.
			if (fldname == "type") {
				return 0;
			}
			if (fldname == "exit_code") {
				return 1;
			}
			return -2;
			break;
	}
	return -1;
}

void qbrt_value::append_type(ostringstream &out, const qbrt_value &val)
{
	switch (val.type->id) {
		case VT_CONSTRUCT:
		case VT_LIST:
			load_construct_value_types(out, *val.data.cons);
			break;
		case VT_FUNCTION:
			load_function_value_types(out, *val.data.f);
			break;
		default:
			out << val.type->module << '/' << val.type->name;
			break;
	}
}

template < typename T >
int type_compare(T a, T b)
{
	if (a < b) {
		return -1;
	} else if (a > b) {
		return 1;
	}
	return 0;
}

int qbrt_compare(const qbrt_value &a, const qbrt_value &b)
{
	if (a.type->id == VT_PATTERNVAR) {
		// if first item is a patternvar, then this is a match
		return 0;
	}

	int comparison(Type::compare(*a.type, *b.type));
	if (comparison) {
		return comparison;
	}

	switch (a.type->id) {
		case VT_INT:
			return type_compare< int64_t >(a.data.i, b.data.i);
		case VT_BOOL:
			return type_compare< bool >(a.data.b, b.data.b);
		case VT_STRING:
			return type_compare< const string & >(
					*a.data.str, *b.data.str);
		case VT_CONSTRUCT:
			return type_compare< const Construct & >(
					*a.data.cons, *b.data.cons);
		default:
			cerr << "Type does not support comparison: "
				<< (int) a.type->id << endl;
			break;
	}
	return 0;
}

string pretty_reg(uint16_t r)
{
	ostringstream result;
	if (REG_IS_PRIMARY(r)) {
		result << 'r' << REG_EXTRACT_PRIMARY(r);
	} else if (REG_IS_SECONDARY(r)) {
		result << 'r' << REG_EXTRACT_SECONDARY1(r)
			<< '.' << REG_EXTRACT_SECONDARY2(r);
	} else if (SPECIAL_REG_RESULT == r) {
		result << "result";
	} else if (CONST_REG_VOID == r) {
		result << "void";
	} else if (SPECIAL_REG_PID == r) {
		result << "pid";
	} else if (CONST_REG_TRUE == r) {
		result << "true";
	} else if (CONST_REG_FALSE == r) {
		result << "false";
	} else {
		result << "wtf? " << r;
	}
	return result.str();
}

void init_instruction_sizes()
{
	INSTRUCTION_SIZE[OP_CALL] = call_instruction::SIZE;
	INSTRUCTION_SIZE[OP_RETURN] = return_instruction::SIZE;
	INSTRUCTION_SIZE[OP_CFAILURE] = cfailure_instruction::SIZE;
	INSTRUCTION_SIZE[OP_CONSTI] = consti_instruction::SIZE;
	INSTRUCTION_SIZE[OP_CONSTS] = consts_instruction::SIZE;
	INSTRUCTION_SIZE[OP_CONSTHASH] = consthash_instruction::SIZE;
	INSTRUCTION_SIZE[OP_FORK] = fork_instruction::SIZE;
	INSTRUCTION_SIZE[OP_IADD] = binaryop_instruction::SIZE;
	INSTRUCTION_SIZE[OP_IDIV] = binaryop_instruction::SIZE;
	INSTRUCTION_SIZE[OP_IMULT] = binaryop_instruction::SIZE;
	INSTRUCTION_SIZE[OP_ISUB] = binaryop_instruction::SIZE;
	INSTRUCTION_SIZE[OP_LCONTEXT] = lcontext_instruction::SIZE;
	INSTRUCTION_SIZE[OP_LCONSTRUCT] = lconstruct_instruction::SIZE;
	INSTRUCTION_SIZE[OP_LFUNC] = lfunc_instruction::SIZE;
	INSTRUCTION_SIZE[OP_LOADTYPE] = loadtype_instruction::SIZE;
	INSTRUCTION_SIZE[OP_LOADOBJ] = loadobj_instruction::SIZE;
	INSTRUCTION_SIZE[OP_MATCH] = match_instruction::SIZE;
	INSTRUCTION_SIZE[OP_MATCHARGS] = matchargs_instruction::SIZE;
	INSTRUCTION_SIZE[OP_MOVE] = move_instruction::SIZE;
	INSTRUCTION_SIZE[OP_REF] = ref_instruction::SIZE;
	INSTRUCTION_SIZE[OP_COPY] = copy_instruction::SIZE;
	INSTRUCTION_SIZE[OP_FIELDGET] = fieldget_instruction::SIZE;
	INSTRUCTION_SIZE[OP_FIELDSET] = fieldset_instruction::SIZE;
	INSTRUCTION_SIZE[OP_GOTO] = goto_instruction::SIZE;
	INSTRUCTION_SIZE[OP_IF] = if_instruction::SIZE;
	INSTRUCTION_SIZE[OP_IFNOT] = if_instruction::SIZE;
	INSTRUCTION_SIZE[OP_IFEQ] = ifcmp_instruction::SIZE;
	INSTRUCTION_SIZE[OP_IFNOTEQ] = ifcmp_instruction::SIZE;
	INSTRUCTION_SIZE[OP_IFFAIL] = iffail_instruction::SIZE;
	INSTRUCTION_SIZE[OP_IFNOTFAIL] = iffail_instruction::SIZE;
	INSTRUCTION_SIZE[OP_CTUPLE] = ctuple_instruction::SIZE;
	INSTRUCTION_SIZE[OP_STUPLE] = stuple_instruction::SIZE;
	INSTRUCTION_SIZE[OP_CLIST] = clist_instruction::SIZE;
	INSTRUCTION_SIZE[OP_CONS] = cons_instruction::SIZE;
	INSTRUCTION_SIZE[OP_NEWPROC] = newproc_instruction::SIZE;
	INSTRUCTION_SIZE[OP_PATTERNVAR] = patternvar_instruction::SIZE;
	INSTRUCTION_SIZE[OP_RECV] = recv_instruction::SIZE;
	INSTRUCTION_SIZE[OP_STRACC] = stracc_instruction::SIZE;
	INSTRUCTION_SIZE[OP_WAIT] = wait_instruction::SIZE;
}

uint8_t isize(uint8_t opcode)
{
	uint8_t sz(INSTRUCTION_SIZE[opcode]);
	if (sz == 0) {
		std::cerr << "Unset instruction size for opcode: "
			<< (int) opcode << std::endl;
		exit(1);
	}
	return sz;
}

void operator & (std::ostream &out, const Death &)
{
	out << endl;
	exit(1);
}

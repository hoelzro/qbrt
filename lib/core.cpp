#include "qbrt/core.h"
#include "qbrt/type.h"
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <unistd.h>
#include "qbrt/arithmetic.h"
#include "qbrt/function.h"
#include "qbrt/logic.h"
#include "qbrt/string.h"
#include "qbrt/tuple.h"
#include "qbrt/list.h"

using namespace std;

static string PRIMITIVE_NAME[256];
uint8_t INSTRUCTION_SIZE[NUM_OP_CODES];

static void init_primitive_names()
{
	PRIMITIVE_NAME[VT_VOID] = "void";
	PRIMITIVE_NAME[VT_INT] = "int";
	PRIMITIVE_NAME[VT_BOOL] = "bool";
	PRIMITIVE_NAME[VT_FLOAT] = "float";
	PRIMITIVE_NAME[VT_FUNCTION] = "function";
	PRIMITIVE_NAME[VT_CFUNCTION] = "function";
	PRIMITIVE_NAME[VT_BSTRING] = "bstring";
	PRIMITIVE_NAME[VT_HASHTAG] = "hashtag";
	PRIMITIVE_NAME[VT_REF] = "ref";
	PRIMITIVE_NAME[VT_TUPLE] = "tuple";
	PRIMITIVE_NAME[VT_LIST] = "list";
	PRIMITIVE_NAME[VT_MAP] = "map";
	PRIMITIVE_NAME[VT_VECTOR] = "vector";
	PRIMITIVE_NAME[VT_PROMISE] = "promise";
	PRIMITIVE_NAME[VT_KIND] = "kind";
	PRIMITIVE_NAME[VT_FAILURE] = "failure";
}

static const std::string & get_primitive_name(uint8_t id)
{
	static bool initialized(false);
	if (!initialized) {
		init_primitive_names();
		initialized = true;
	}
	return PRIMITIVE_NAME[id];
}

Type::Type(uint8_t id)
	: module("qbrt")
	, name(get_primitive_name(id))
	, id(id)
{}

Type::Type(const std::string &mod, const std::string &name, uint16_t flds)
	: module(mod)
	, name(name)
	, id(VT_STRUCT)
{}

Type TYPE_VOID(VT_VOID);
Type TYPE_KIND(VT_KIND);
Type TYPE_INT(VT_INT);
Type TYPE_BSTRING(VT_BSTRING);
Type TYPE_BOOL(VT_BOOL);
Type TYPE_FLOAT(VT_FLOAT);
Type TYPE_HASHTAG(VT_HASHTAG);
Type TYPE_REF(VT_REF);
Type TYPE_TUPLE(VT_TUPLE);
Type TYPE_FUNCTION(VT_FUNCTION);
Type TYPE_CFUNCTION(VT_CFUNCTION);
Type TYPE_LIST(VT_LIST);
Type TYPE_MAP(VT_MAP);
Type TYPE_VECTOR(VT_VECTOR);
Type TYPE_STREAM(VT_STREAM);
Type TYPE_PROMISE(VT_PROMISE);
Type TYPE_FAILURE(VT_FAILURE);

void qbrt_value::set_void(qbrt_value &v)
{
	switch (v.type->id) {
		case VT_BSTRING:
			delete v.data.str;
			v.data.str = NULL;
			break;
		case VT_HASHTAG:
			delete v.data.hashtag;
			v.data.hashtag = NULL;
			break;
		case VT_BOOL:
		case VT_INT:
		case VT_FLOAT:
		case VT_VOID:
			break;
	}
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
	if (a.type->id < b.type->id) {
		return -1;
	} else if (a.type->id > b.type->id) {
		return 1;
	}
	switch (a.type->id) {
		case VT_INT:
			return type_compare< int64_t >(a.data.i, b.data.i);
		case VT_BSTRING:
			return type_compare< const string & >(*a.data.str, *b.data.str);
	}
	return 0;
}

string pretty_reg(uint16_t r)
{
	ostringstream result;
	if (REG_IS_PRIMARY(r)) {
		result << "r";
		result << (int) (r & 0x007f);
	} else if (REG_IS_SECONDARY(r)) {
		result << "r";
		result << (int) ((r & 0x7f00) >> 8);
		result << ".r";
		result << (int) (r & 0x7f);
	} else if (SPECIAL_REG_RESULT == r) {
		result << "result";
	} else if (CONST_REG_VOID == r) {
		result << "void";
	} else {
		result << "wtf? " << r;
	}
	return result.str();
}

void init_instruction_sizes()
{
	INSTRUCTION_SIZE[OP_ADDI] = binaryop_instruction::SIZE;
	INSTRUCTION_SIZE[OP_CALL] = call_instruction::SIZE;
	INSTRUCTION_SIZE[OP_RETURN] = return_instruction::SIZE;
	INSTRUCTION_SIZE[OP_CFAILURE] = cfailure_instruction::SIZE;
	INSTRUCTION_SIZE[OP_CONSTI] = consti_instruction::SIZE;
	INSTRUCTION_SIZE[OP_CONSTS] = consts_instruction::SIZE;
	INSTRUCTION_SIZE[OP_CONSTHASH] = consthash_instruction::SIZE;
	INSTRUCTION_SIZE[OP_FORK] = fork_instruction::SIZE;
	INSTRUCTION_SIZE[OP_IDIV] = binaryop_instruction::SIZE;
	INSTRUCTION_SIZE[OP_IMULT] = binaryop_instruction::SIZE;
	INSTRUCTION_SIZE[OP_ISUB] = binaryop_instruction::SIZE;
	INSTRUCTION_SIZE[OP_LFUNC] = lfunc_instruction::SIZE;
	INSTRUCTION_SIZE[OP_LCONTEXT] = lcontext_instruction::SIZE;
	INSTRUCTION_SIZE[OP_LOADTYPE] = loadtype_instruction::SIZE;
	INSTRUCTION_SIZE[OP_LOADOBJ] = loadobj_instruction::SIZE;
	INSTRUCTION_SIZE[OP_MOVE] = move_instruction::SIZE;
	INSTRUCTION_SIZE[OP_REF] = ref_instruction::SIZE;
	INSTRUCTION_SIZE[OP_COPY] = copy_instruction::SIZE;
	INSTRUCTION_SIZE[OP_GOTO] = goto_instruction::SIZE;
	INSTRUCTION_SIZE[OP_BRF] = brbool_instruction::SIZE;
	INSTRUCTION_SIZE[OP_BRT] = brbool_instruction::SIZE;
	INSTRUCTION_SIZE[OP_BREQ] = brcmp_instruction::SIZE;
	INSTRUCTION_SIZE[OP_BRNE] = brcmp_instruction::SIZE;
	INSTRUCTION_SIZE[OP_BRFAIL] = brfail_instruction::SIZE;
	INSTRUCTION_SIZE[OP_BRNFAIL] = brfail_instruction::SIZE;
	INSTRUCTION_SIZE[OP_CTUPLE] = ctuple_instruction::SIZE;
	INSTRUCTION_SIZE[OP_STUPLE] = stuple_instruction::SIZE;
	INSTRUCTION_SIZE[OP_CLIST] = clist_instruction::SIZE;
	INSTRUCTION_SIZE[OP_CONS] = cons_instruction::SIZE;
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


void StreamReadline::handle()
{
	char *line = NULL;
	size_t n(0);
	size_t len(getline(&line, &n, file));
	// get rid of that lame newline
	// I will probably need to undue this later
	if (line[len-1] == '\n') {
		line[len-1] = '\0';
	}
	dst = line;
	free(line);
}

void StreamWrite::handle()
{
	size_t result(write(fd, src.c_str(), src.size()));
	if (result < 0) {
		cerr << "failure of some kind\n";
	}
}

size_t Stream::read(std::string &result)
{
	char buffer[1024];
	fgets(buffer, sizeof(buffer), file);
	result = buffer;
}

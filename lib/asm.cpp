
#include <list>
#include <map>
#include <set>
#include <stdlib.h>
#include <vector>
#include <errno.h>
#include <iostream>
#include <sstream>
#include <memory>
#include <sys/stat.h>
#include <fcntl.h>
#include "qbrt/core.h"
#include "qbrt/string.h"
#include "qbrt/arithmetic.h"
#include "qbrt/function.h"
#include "qbrt/logic.h"
#include "qbrt/module.h"
#include "qbrt/list.h"
#include "qbrt/tuple.h"
#include "instruction/schedule.h"
#include "qbtoken.h"
#include "qbparse.h"
#include "asm.h"

using namespace std;

Stmt::List *parsed_stmts;
uint16_t AsmResource::NULL_INDEX = 0;

typedef std::list< ResourceInfo > ResourceIndex;
void generate_codeblock(AsmFunc &, const Stmt::List &);

struct ObjectBuilder
{
	ObjectHeader header;
	ResourceSet rs;
};


void asm_instruction(AsmFunc &f, instruction *i)
{
	f.code.push_back(i);
}

void asm_jump(AsmFunc &f, const string &lbl, jump_instruction *j)
{
	f.jump.push_back(jump_data(*j, lbl, f.code.size()));
	asm_instruction(f, j);
}

void label_next(AsmFunc &f, const string &lbl)
{
	label_map::const_iterator it(f.label.find(lbl));
	if (it != f.label.end()) {
		cerr << "duplicate label: " << lbl << " in function "
			<< f.name.value << endl;
		exit(1);
	}
	f.label[lbl] = f.code.size();
}


uint32_t label_index(const label_map &lmap, const std::string &lbl)
{
	label_map::const_iterator it(lmap.find(lbl));
	if (it == lmap.end()) {
		cerr << "unknown label: " << lbl << endl;
		return -1;
	}
	return it->second;
}


template < typename T >
int compare_asmptr(const T *a, const T *b)
{
	if (!(a || b)) {
		return 0;
	}
	if (!a) {
		return -1;
	}
	if (!b) {
		return 1;
	}
	return AsmCmp< T >::cmp(*a, *b);
}

template <>
int AsmCmp< AsmString >::cmp(const AsmString &a, const AsmString &b)
{
	if (a.value.empty() && b.value.empty()) {
		return 0;
	}
	if (a.value.empty()) {
		return -1;
	}
	if (b.value.empty()) {
		return 1;
	}
	if (a.value < b.value) {
		return -1;
	}
	if (a.value > b.value) {
		return 1;
	}
	return 0;
}

template <>
int AsmCmp< AsmHashTag >::cmp(const AsmHashTag &a, const AsmHashTag &b)
{
	if (a.value < b.value) {
		return -1;
	}
	if (a.value > b.value) {
		return 1;
	}
	return 0;
}

template <>
int AsmCmp< AsmModSym >::cmp(const AsmModSym &a, const AsmModSym &b)
{
	int comparison(AsmCmp< AsmString >::cmp(a.module, b.module));
	if (comparison) {
		return comparison;
	}
	return AsmCmp< AsmString >::cmp(a.symbol, b.symbol);
}

template <>
int AsmCmp< AsmModSymList >::cmp(const AsmModSymList &a, const AsmModSymList &b)
{
	AsmModSymList::const_iterator x(a.begin());
	AsmModSymList::const_iterator y(b.begin());
	int comparison;
	for (;;) {
		if (x == a.end() && y == b.end()) {
			break;
		}
		if (x == a.end()) {
			return -1;
		}
		if (y == b.end()) {
			return 1;
		}
		comparison = compare_asmptr< AsmModSym >(*x, *y);
		if (comparison) {
			return comparison;
		}
	}
	return 0;
}

template <>
int AsmCmp< AsmProtocol >::cmp(const AsmProtocol &a, const AsmProtocol &b)
{
	return AsmCmp< AsmString >::cmp(a.name, b.name);
}

template <>
int AsmCmp< AsmPolymorph >::cmp(const AsmPolymorph &a, const AsmPolymorph &b)
{
	int comparison(AsmCmp< AsmModSym >::cmp(a.protocol, b.protocol));
	if (comparison) {
		return comparison;
	}

	return AsmCmp< AsmModSymList >::cmp(a.type, b.type);
}

template <>
int AsmCmp< AsmFunc >::cmp(const AsmFunc &a, const AsmFunc &b)
{
	int comparison(compare_asmptr< AsmResource >(a.ctx, b.ctx));
	if (comparison) {
		return comparison;
	}
	return AsmCmp< AsmString >::cmp(a.name, b.name);
}

template < typename T >
int compare_asm(const AsmResource &a, const AsmResource &b)
{
	const T &x(static_cast< const T & >(a));
	const T &y(static_cast< const T & >(b));
	return AsmCmp< T >::cmp(x, y);
}

template <>
int AsmCmp< AsmResource >::cmp(const AsmResource &a, const AsmResource &b)
{
	if (a.type < b.type) {
		return -1;
	}
	if (a.type > b.type) {
		return 1;
	}

	switch (a.type) {
		case RESOURCE_STRING:
			return compare_asm< AsmString >(a, b);
		case RESOURCE_MODSYM:
			return compare_asm< AsmModSym >(a, b);
		case RESOURCE_FUNCTION:
			return compare_asm< AsmFunc >(a, b);
		case RESOURCE_HASHTAG:
			return compare_asm< AsmHashTag >(a, b);
		case RESOURCE_PROTOCOL:
			return compare_asm< AsmProtocol >(a, b);
		case RESOURCE_POLYMORPH:
			return compare_asm< AsmPolymorph >(a, b);
	}
	cerr << "what type is this? not supported man.\n";
	return 0;
}

bool ResourceLess::operator() (const AsmResource *a, const AsmResource *b) const
{
	return compare_asmptr(a, b) < 0;
}


void RegAlloc::alloc(AsmReg &reg)
{
	if (reg.idx >= 0) {
		return;
	}
	switch (reg.type) {
		case '%':
			alloc_arg(reg);
			break;
		case '$':
			alloc_reg(reg);
			break;
		case 'c':
		case 's':
			// these register types are preallocated
			return;
	}
}

void RegAlloc::alloc_arg(AsmReg &reg)
{
	CountMap::const_iterator it(args.find(reg.name));
	if (it != args.end()) {
		reg.idx = it->second;
		return;
	}
	istringstream name(reg.name);
	int reg_idx;
	name >> reg_idx;
	args[reg.name] = reg.idx = reg_idx;
}

void RegAlloc::alloc_reg(AsmReg &reg)
{
	CountMap::const_iterator it(registry.find(reg.name));
	if (it != registry.end()) {
		reg.idx = it->second;
		return;
	}
	reg.idx = counter++;
	registry[reg.name] = reg.idx;
}


AsmReg::operator reg_t () const
{
	reg_t id(0);
	switch (type) {
		case '%':
		case '$':
			if (ext < 0) {
				id = PRIMARY_REG(idx);
			} else {
				id = SECONDARY_REG(idx, ext);
			}
			break;
		case 'c':
		case 's':
			id = specialid;
			break;
	}
	return id;
}

ostream & operator << (ostream &o, const AsmReg &r)
{
	switch (r.type) {
		case '%':
		case '$':
			if (r.ext < 0) {
				o << r.type << (int) r.idx;
			} else {
				o << r.type << (int) r.idx
					<< '.' << (int) r.ext;
			}
			break;
		case 'c':
		case 's':
			o << r.type << r.specialid;
			break;
		default:
			cerr << "Unknown register type code: "
				<< r.type << endl;
			break;
	}
	return o;
}

AsmReg * AsmReg::parse_arg(const std::string &t)
{
	istringstream in(t);
	char typecode(in.get()); // should be '%'
	AsmReg *r = new AsmReg(typecode);
	in >> r->name;
	return r;
}

AsmReg * AsmReg::parse_reg(const std::string &t)
{
	istringstream in(t);
	char typecode(in.get()); // should be '$'
	AsmReg *r = new AsmReg(typecode);
	in >> r->name;
	return r;
}

AsmReg * AsmReg::parse_extended_arg(const std::string &arg
		, const std::string &ext)
{
	AsmReg *r = AsmReg::parse_arg(arg);
	r->ext = AsmReg::parse_regext(ext);
	return r;
}

AsmReg * AsmReg::parse_extended_reg(const std::string &reg
		, const std::string &ext)
{
	AsmReg *r = AsmReg::parse_reg(reg);
	r->ext = AsmReg::parse_regext(ext);
	return r;
}

int AsmReg::parse_regext(const std::string &ext)
{
	istringstream in(ext);
	in.get(); // skip the leading .
	int ext_idx;
	in >> ext_idx;
	return ext_idx;
}

AsmReg * AsmReg::create_special(uint16_t id)
{
	AsmReg *r = new AsmReg('s');
	r->specialid = SPECIAL_REG(id);
	return r;
}


bool collect_resource(ResourceSet &rs, AsmResource &res)
{
	pair< AsmResource *, uint16_t > rp(&res, 0);
	pair< map< AsmResource *, uint16_t >::iterator, bool > result;
	result = rs.insert(rp);
	res.index = &result.first->second;
	return result.second;
}

bool collect_string(ResourceSet &rs, AsmString &str)
{
	bool result(collect_resource(rs, str));
if (result) {
  cout << "collect_string(" << str.value << ")\n";
}
	return result;
}

bool collect_modsym(ResourceSet &rs, AsmModSym &modsym)
{
	collect_string(rs, modsym.module);
	collect_string(rs, modsym.symbol);
	bool added(collect_resource(rs, modsym));
if (added) {
  cout << "collect_modsym(" << modsym.module.value << ',' << modsym.symbol.value << ")\n";
}
	return added;
}


typedef uint8_t (*instruction_writer)(ostream &, const instruction &);

int16_t count_jump_bytes(codeblock::const_iterator &it, int16_t jump_count
		, int16_t jump_bytes)
{
	if (jump_count < 0) {
		jump_bytes -= isize(**(--it));
		return count_jump_bytes(it, ++jump_count, jump_bytes);
	} else if (jump_count > 0) {
		jump_bytes += isize(**(it++));
		return count_jump_bytes(it, --jump_count, jump_bytes);
	}
	return jump_bytes;
}

int16_t count_jump_bytes(codeblock::iterator &it)
{
	jump_instruction *jumpi = (jump_instruction *) (*it);
	codeblock::const_iterator cit(it);
	int16_t jmp(count_jump_bytes(cit, jumpi->jump(), 0));
	jumpi->setjump(jmp);
	return jmp;
}

void measure_jump_bytes(codeblock &code)
{
	codeblock::iterator it;
	for (it=code.begin(); it!=code.end(); ++it) {
		switch ((*it)->opcode()) {
			case OP_GOTO:
			case OP_BRF:
			case OP_BRFAIL:
			case OP_BRT:
			case OP_BREQ:
			case OP_BRNE:
			case OP_BRNFAIL:
			case OP_FORK:
				count_jump_bytes(it);
				break;
			default:
				continue;
		}
	}
}

void measure_function_jumps(AsmFunc &f)
{
	list< jump_data >::iterator jump_it(f.jump.begin());
	for (; jump_it!=f.jump.end(); ++jump_it) {
		uint32_t lbl_idx = label_index(f.label, jump_it->label);
		int16_t jump_count(lbl_idx - jump_it->jump_offset);
		jump_it->ji.setjump(jump_count);
	}

	measure_jump_bytes(f.code);
}

void measure_jumps(ResourceSet &rs)
{
	ResourceSet::iterator it(rs.begin());
	for (; it!=rs.end(); ++it) {
		if (it->first->type == RESOURCE_FUNCTION) {
			AsmFunc *func = dynamic_cast< AsmFunc * >(it->first);
			measure_function_jumps(*func);
		}
	}
}

template < typename I >
uint8_t iwriter(ostream &out, const I &i)
{
	out.write((char *) &i, I::SIZE);
	return I::SIZE;
}

#define DEFINE_IWRITER(i) \
	template uint8_t iwriter< i##_instruction >(ostream & \
			, const i##_instruction &);

DEFINE_IWRITER(binaryop);
DEFINE_IWRITER(consti);
DEFINE_IWRITER(consts);
DEFINE_IWRITER(consthash);
DEFINE_IWRITER(call);
DEFINE_IWRITER(fork);
DEFINE_IWRITER(lfunc);
DEFINE_IWRITER(lcontext);
DEFINE_IWRITER(loadtype);
DEFINE_IWRITER(loadobj);
DEFINE_IWRITER(lpfunc);
DEFINE_IWRITER(newproc);
DEFINE_IWRITER(recv);
DEFINE_IWRITER(stracc);
DEFINE_IWRITER(unimorph);
DEFINE_IWRITER(move);
DEFINE_IWRITER(ref);
DEFINE_IWRITER(copy);
DEFINE_IWRITER(return);
DEFINE_IWRITER(goto);
DEFINE_IWRITER(brbool);
DEFINE_IWRITER(brcmp);
DEFINE_IWRITER(breq);
DEFINE_IWRITER(brfail);
DEFINE_IWRITER(ctuple);
DEFINE_IWRITER(stuple);
DEFINE_IWRITER(clist);
DEFINE_IWRITER(cons);

instruction_writer WRITER[NUM_OP_CODES] = {0};

void init_writers()
{
	WRITER[OP_IADD] = (instruction_writer) iwriter<binaryop_instruction>;
	WRITER[OP_CALL] = (instruction_writer) iwriter<call_instruction>;
	WRITER[OP_RETURN] = (instruction_writer) iwriter<return_instruction>;
	WRITER[OP_CFAILURE] =
		(instruction_writer) iwriter<cfailure_instruction>;
	WRITER[OP_CONSTI] = (instruction_writer) iwriter<consti_instruction>;
	WRITER[OP_CONSTS] = (instruction_writer) iwriter<consts_instruction>;
	WRITER[OP_CONSTHASH] =
		(instruction_writer) iwriter<consthash_instruction>;
	WRITER[OP_LCONTEXT] = (instruction_writer)iwriter<lcontext_instruction>;
	WRITER[OP_FORK] = (instruction_writer) iwriter<fork_instruction>;
	WRITER[OP_IDIV] = (instruction_writer) iwriter<binaryop_instruction>;
	WRITER[OP_IMULT] = (instruction_writer) iwriter<binaryop_instruction>;
	WRITER[OP_ISUB] = (instruction_writer) iwriter<binaryop_instruction>;
	WRITER[OP_LFUNC] = (instruction_writer)iwriter<lfunc_instruction>;
	WRITER[OP_LOADTYPE] = (instruction_writer)iwriter<loadtype_instruction>;
	WRITER[OP_LOADOBJ] = (instruction_writer)iwriter<loadobj_instruction>;
	WRITER[OP_LPFUNC] = (instruction_writer)iwriter<lpfunc_instruction>;
	WRITER[OP_NEWPROC] = (instruction_writer)iwriter<newproc_instruction>;
	WRITER[OP_RECV] = (instruction_writer)iwriter<recv_instruction>;
	WRITER[OP_STRACC] = (instruction_writer)iwriter<stracc_instruction>;
	WRITER[OP_UNIMORPH] = (instruction_writer)iwriter<unimorph_instruction>;
	WRITER[OP_MOVE] = (instruction_writer) iwriter<move_instruction>;
	WRITER[OP_REF] = (instruction_writer) iwriter<ref_instruction>;
	WRITER[OP_COPY] = (instruction_writer) iwriter<copy_instruction>;
	WRITER[OP_GOTO] = (instruction_writer) iwriter<goto_instruction>;
	WRITER[OP_BRF] = (instruction_writer) iwriter<brbool_instruction>;
	WRITER[OP_BRT] = (instruction_writer) iwriter<brbool_instruction>;
	WRITER[OP_BRNE] = (instruction_writer) iwriter<brcmp_instruction>;
	WRITER[OP_BREQ] = (instruction_writer) iwriter<breq_instruction>;
	WRITER[OP_BRFAIL] = (instruction_writer) iwriter<brfail_instruction>;
	WRITER[OP_BRNFAIL] = (instruction_writer) iwriter<brfail_instruction>;
	WRITER[OP_CTUPLE] = (instruction_writer) iwriter<ctuple_instruction>;
	WRITER[OP_STUPLE] = (instruction_writer) iwriter<stuple_instruction>;
	WRITER[OP_CLIST] = (instruction_writer) iwriter<clist_instruction>;
	WRITER[OP_CONS] = (instruction_writer) iwriter<cons_instruction>;
	WRITER[OP_WAIT] = (instruction_writer) iwriter<wait_instruction>;
}

map< string, uint16_t > CONSTNUM;
map< string, uint16_t > CONSTSTR;
map< string, uint16_t > CONSTKEY;

void init_constant_registers()
{
	CONSTNUM["0"] = REG_CINT(0x0);
	CONSTNUM["1"] = REG_CINT(0x1);
	CONSTNUM["2"] = REG_CINT(0x2);
	CONSTNUM["3"] = REG_CINT(0x3);
	CONSTNUM["4"] = REG_CINT(0x4);
	CONSTNUM["5"] = REG_CINT(0x5);
	CONSTNUM["6"] = REG_CINT(0x6);
	CONSTNUM["7"] = REG_CINT(0x7);
	CONSTNUM["8"] = REG_CINT(0x8);
	CONSTNUM["9"] = REG_CINT(0x9);
	CONSTNUM["10"] = REG_CINT(0xa);
	CONSTNUM["11"] = REG_CINT(0xb);
	CONSTNUM["12"] = REG_CINT(0xc);
	CONSTNUM["13"] = REG_CINT(0xd);
	CONSTNUM["14"] = REG_CINT(0xe);
	CONSTNUM["15"] = REG_CINT(0xf);
	CONSTNUM["0.0"] = REG_FZERO;
	CONSTSTR[""] = REG_EMPTYSTR;
	CONSTSTR["\n"] = REG_NEWLINE;
	CONSTKEY["false"] = REG_FALSE;
	CONSTKEY["true"] = REG_TRUE;
	CONSTKEY["void"] = REG_VOID;
}

uint8_t write_instruction(ostream &out, const instruction &i)
{
	instruction_writer w = WRITER[i.opcode()];
	if (!w) {
		cerr << "instruction writer is null for: " << (int) i.opcode()
			<< endl;
	}
	return w(out, i);
}

/**
 * Write the header in byte packed format
 */
void write_header(ostream &out, const ObjectHeader &h)
{
	out.write("qbrt", 4);
	out.write((const char *) &h.qbrt_version, 4);
	out.write((const char *) &h.flags.raw, 8);
	out.write(h.library_name, 24);
	out.write((const char *) &h.library_version, 2);
	out.write(h.library_iteration, 6);
}

/**
 * Write the resource data in byte packed format
 */
void write_resource_header(ostream &out, const ResourceTableHeader &h)
{
	uint16_t count_plus_1 = h.resource_count + 1;
	out.write((char *) &h.data_size, 4);
	out.write((char *) &count_plus_1, 2);
}

bool AsmFunc::has_code() const
{
	return !this->code.empty();
}

uint8_t AsmFunc::fcontext() const
{
	return this->has_code() ? FCT_WITH_CODE(ctx_type) : ctx_type;
}

uint32_t AsmFunc::write(ostream &out) const
{
	out.write((const char *) name.index, 2);
	out.write((const char *) doc.index, 2);
	out.write((const char *) filename.index, 2);
	out.write((const char *) (&line_no), 2);
	if (ctx) {
		out.write((const char *) ctx->index, 2);
	} else {
		uint16_t zero(0);
		out.write((const char *) &zero, 2);
	}
	out.put(this->fcontext());
	out.put(argc);
	out.put(regc);
	uint32_t func_size(FunctionResource::HEADER_SIZE);

	codeblock::const_iterator ci(code.begin());
	for (; ci!=code.end(); ++ci) {
		uint8_t isize(write_instruction(out, **ci));
		cout << "opcode=" << (int) (*ci)->opcode();
		cout << " isize=" << (int) isize << endl;
		func_size += isize;
	}
	return func_size;
}

uint32_t AsmProtocol::write(ostream &out) const
{
	uint16_t num_funcs(0);
	if (num_funcs >= (1 << 10)) {
		cerr << "oh shit that's a lot of functions" << endl;
	}
	uint16_t arg_func_counts((argc << 10) | num_funcs);
	endian_fix(arg_func_counts);

	out.write((const char *) name.index, 2);
	out.write((const char *) doc.index, 2);
	out.write((const char *) filename.index, 2);
	out.write((const char *) &line_no, 2);
	out.write((const char *) &arg_func_counts, 2);

	uint32_t proto_size(10);

	return proto_size;
}

uint32_t AsmPolymorph::write(ostream &out) const
{
	uint16_t num_types(type.size());
	uint16_t num_funcs(0);

	out.write((const char *) protocol.index, 2);
	out.write((const char *) doc.index, 2);
	out.write((const char *) filename.index, 2);
	out.write((const char *) &line_no, 2);
	out.write((const char *) &num_types, 2);
	out.write((const char *) &num_funcs, 2);

	uint32_t poly_size(12);
	list< AsmModSym * >::const_iterator it(type.begin());
	for (; it!=type.end(); ++it) {
		out.write((const char *) (*it)->index, 2);
		poly_size += 2;
	}
	return poly_size;
}

std::ostream & AsmPolymorph::pretty(std::ostream &o) const
{
	o << "polymorph:" << protocol.module.value
		<< "." << protocol.symbol.value;
	return o;
}


void write_resource_data(ostream &out, ResourceIndex &index
		, const ResourceSet &rs)
{
	uint32_t offset(0);
	ResourceSet::const_iterator it(rs.begin());
	for (; it!=rs.end(); ++it) {
		index.push_back(ResourceInfo(offset, it->first->type));
		offset += it->first->write(out);
	}
}

void write_resource_index(ostream &out, const ResourceIndex &index)
{
	ResourceInfo null_resource(0, 0);
	out.write((const char *) &null_resource, ResourceInfo::SIZE);

	ResourceIndex::const_iterator it(index.begin());
	for (; it!=index.end(); ++it) {
		out.write((const char *) &(*it), ResourceInfo::SIZE);
	}
}

ResourceTableHeader write_resource_table(ostream &out, const ResourceSet &rs)
{
	ResourceIndex index;
	uint32_t pre_data(out.tellp());
	write_resource_data(out, index, rs);
	uint32_t post_data(out.tellp());
	write_resource_index(out, index);

	ResourceTableHeader tbl_header;
	tbl_header.data_size = post_data - pre_data;
	tbl_header.resource_count = rs.size();
	return tbl_header;
}

void write_object(ostream &out, ObjectBuilder &obj)
{
	ResourceTableHeader tbl_header;

	write_header(out, obj.header);
	write_resource_header(out, tbl_header);
	tbl_header = write_resource_table(out, obj.rs);

	// and rewrite the header w/ correct offsets & sizes
	out.seekp(0);
	write_header(out, obj.header);
	write_resource_header(out, tbl_header);
}


ostream & operator << (ostream &out, const Token &t)
{
	switch (t.type) {
		case TOKEN_CALL:
			out << "call";
			break;
		case TOKEN_CONST:
			out << "const";
			break;
		case TOKEN_END:
			out << "end.";
			break;
		case TOKEN_FORK:
			out << "fork";
			break;
		case TOKEN_INT:
			out << "INT";
			break;
		case TOKEN_LABEL:
			out << "LABEL";
			break;
		case TOKEN_LFUNC:
			out << "lfunc";
			break;
		case TOKEN_LCONTEXT:
			out << "lcontext";
			break;
		case TOKEN_NEWPROC:
			out << "newproc";
			break;
		case TOKEN_RECV:
			out << "recv";
			break;
		case TOKEN_STR:
			out << "STRING";
			break;
		case TOKEN_REF:
			out << "ref";
			break;
		case TOKEN_REG:
			out << "REG";
			break;
		case TOKEN_REGEXT:
			out << "REGEXT";
			break;
		case TOKEN_RESULT:
			out << "result";
			break;
		case TOKEN_PARAM:
			out << "PARAM";
			break;
		case TOKEN_STRACC:
			out << "stracc";
			break;
		default:
			out << t.type;
			break;
	}
	return out;
}

class TokenState
{
public:
	TokenState(int32_t lineno, const std::string &line)
		: lineno(lineno)
		, p(line.begin())
		, end(line.end())
	{}
	bool eol() const { return p == end; }
	bool space() const { return isspace(*p); }
	bool digit() const { return isdigit(*p); }
	char operator * () { return *p; }
	void operator ++ () { ++p; ++col; }
	const int32_t lineno;
	int32_t colno() const { return col; }

private:
	string::const_iterator p;
	string::const_iterator end;
	int32_t col;
};

void print_tokens(ostream &out, const list< vector< Token * > > &file)
{
	cout << "num lines: " << file.size() << endl;
	list< vector< Token * > >::const_iterator line(file.begin());
	for (; line!=file.end(); ++line) {
		cout << line->size() << ":";
		vector< Token * >::const_iterator tok(line->begin());
		for (; tok!=line->end(); ++tok) {
			out << " " << **tok;
		}
		out << endl;
	}
}


void brbool_stmt::allocate_registers(RegAlloc *alloc)
{
	alloc->alloc(*reg);
}

void brbool_stmt::generate_code(AsmFunc &f)
{
	asm_jump(f, label.name, new brbool_instruction(check, 0, *reg));
}

void brfail_stmt::allocate_registers(RegAlloc *alloc)
{
	alloc->alloc(*reg);
}

void brfail_stmt::generate_code(AsmFunc &f)
{
	asm_jump(f, label.name, new brfail_instruction(check, 0, *reg));
}

void binaryop_stmt::allocate_registers(RegAlloc *alloc)
{
	alloc->alloc(*result);
	alloc->alloc(*a);
	alloc->alloc(*b);
}

void binaryop_stmt::generate_code(AsmFunc &f)
{
	uint16_t cmp;
	((char *) &cmp)[0] = type;
	((char *) &cmp)[1] = op;
	uint8_t opcode(0);
	switch (cmp) {
		case 0x2b69: // +i
			opcode = OP_IADD;
			break;
		case 0x2d69: // -i
			opcode = OP_ISUB;
			break;
		case 0x2a69: // *i
			opcode = OP_IMULT;
			break;
		case 0x2f69: // /i
			opcode = OP_IDIV;
			break;
		default:
			std::cerr << "unknown binary op: " << type << op
				<<' '<< cmp << endl;
			break;
	}
	asm_instruction(f, new binaryop_instruction(opcode, *result, *a, *b));
}

brcmp_stmt * brcmp_stmt::ne(AsmReg *a, AsmReg *b, const std::string &lbl)
{
	return new brcmp_stmt(OP_BRNE, a, b, lbl);
}

void brcmp_stmt::allocate_registers(RegAlloc *r)
{
	r->alloc(*a);
	r->alloc(*b);
}

void brcmp_stmt::generate_code(AsmFunc &f)
{
	asm_jump(f, label.name, new brcmp_instruction(opcode, *a, *b));
}

void call_stmt::allocate_registers(RegAlloc *r)
{
	r->alloc(*result);
	r->alloc(*function);
}

void call_stmt::generate_code(AsmFunc &f)
{
	asm_instruction(f, new call_instruction(*result, *function));
}

void cfailure_stmt::collect_resources(ResourceSet &rs)
{
	collect_resource(rs, type);
}

void cfailure_stmt::generate_code(AsmFunc &f)
{
	asm_instruction(f, new cfailure_instruction(*type.index));
}

void consti_stmt::allocate_registers(RegAlloc *r)
{
	r->alloc(*dst);
}

void consti_stmt::generate_code(AsmFunc &f)
{
	asm_instruction(f, new consti_instruction(*dst, value));
}

void consts_stmt::allocate_registers(RegAlloc *r)
{
	r->alloc(*dst);
}

void consts_stmt::collect_resources(ResourceSet &rs)
{
	collect_string(rs, value);
}

void consts_stmt::generate_code(AsmFunc &f)
{
	asm_instruction(f, new consts_instruction(*dst, *value.index));
}

void consthash_stmt::allocate_registers(RegAlloc *r)
{
	r->alloc(*dst);
}

void consthash_stmt::collect_resources(ResourceSet &rs)
{
	collect_resource(rs, value);
}

void consthash_stmt::generate_code(AsmFunc &f)
{
	asm_instruction(f, new consthash_instruction(*dst, *value.index));
}

void copy_stmt::allocate_registers(RegAlloc *r)
{
	r->alloc(*dst);
	r->alloc(*src);
}

void copy_stmt::generate_code(AsmFunc &f)
{
	asm_instruction(f, new copy_instruction(*dst, *src));
}

struct ClistStatement
{
	ClistStatement(uint16_t dst)
		: dst(dst)
	{}

	uint16_t dst;

	void pretty(ostream &out) const
	{
		out << "clist " << dst;
	}
};

struct ConsStatement
{
	ConsStatement(uint16_t head, uint16_t item)
		: head(head)
		, item(item)
	{}

	uint16_t head;
	uint16_t item;

	void pretty(ostream &out) const
	{
		out << "cons " << head <<" "<< item;
	}
};

void dfunc_stmt::set_function_context(uint8_t afc, AsmResource *ctx)
{
	AsmFunc *f = new AsmFunc(this->name);
	f->argc = arity;
	f->regc = 0;
	f->stmts = code;
	f->ctx = ctx;
	f->ctx_type = afc;
	this->func = f;
}

void dfunc_stmt::allocate_registers(RegAlloc *)
{
	if (!code) {
		return;
	}
	RegAlloc regs(func->argc);
	Stmt::List::iterator it(code->begin());
	for (; it!=code->end(); ++it) {
		(*it)->allocate_registers(&regs);
	}
	func->regc = regs.counter - func->argc;
}

void fork_stmt::allocate_registers(RegAlloc *ra)
{
	ra->alloc(*dst);
	if (code) {
		::allocate_registers(*code, ra);
	}
}

void fork_stmt::collect_resources(ResourceSet &rs)
{
	if (code) {
		::collect_resources(rs, *code);
	}
}

void fork_stmt::generate_code(AsmFunc &f)
{
	if (!code) {
		return;
	}
	asm_jump(f, "postfork", new fork_instruction(0, *dst));
	::generate_codeblock(f, *code);
	asm_instruction(f, new return_instruction);
	label_next(f, "postfork");
}

void dprotocol_stmt::allocate_registers(RegAlloc *alloc)
{
	if (!functions) {
		return;
	}
	Stmt::List::iterator it(functions->begin());
	for (; it!=functions->end(); ++it) {
		(*it)->allocate_registers(alloc);
	}
}

void dprotocol_stmt::set_function_context(uint8_t, AsmResource *)
{
	this->protocol = new AsmProtocol(name);
	this->protocol->argc = arity;

	::set_function_context(*functions, FCT_PROTOCOL, this->protocol);
}

void dpolymorph_stmt::allocate_registers(RegAlloc *alloc)
{
	if (!functions) {
		return;
	}
	Stmt::List::iterator it(functions->begin());
	for (; it!=functions->end(); ++it) {
		(*it)->allocate_registers(alloc);
	}
}

void dpolymorph_stmt::set_function_context(uint8_t, AsmResource *)
{
	Stmt::List::iterator it(morph_stmts->begin());
	this->morph_types = new AsmModSymList();
	morphtype_stmt *mt;
	for (; it!=morph_stmts->end(); ++it) {
		mt = dynamic_cast< morphtype_stmt * >(*it);
		this->morph_types->push_back(mt->type);
	}

	this->polymorph = new AsmPolymorph(*protocol);
	this->polymorph->type = *this->morph_types;

	::set_function_context(*functions, FCT_POLYMORPH, this->polymorph);
}

/**
 * wtf is this function trying to do?
 * I think it's trying to assign protocol info for functions
 */
void set_function_context(Stmt::List &funcs, uint8_t pfc
		, AsmResource *ctx)
{
	Stmt::List::iterator it(funcs.begin());
	for (; it!=funcs.end(); ++it) {
		(*it)->set_function_context(pfc, ctx);
	}
}

void allocate_registers(Stmt::List &stmts, RegAlloc *rc)
{
	Stmt::List::iterator it(stmts.begin());
	for (; it!=stmts.end(); ++it) {
		(*it)->allocate_registers(rc);
	}
}

void dfunc_stmt::collect_resources(ResourceSet &rs)
{
	collect_string(rs, name);
	if (code) {
		::collect_resources(rs, *code);
	}
	collect_resource(rs, *this->func);
}

void dprotocol_stmt::collect_resources(ResourceSet &rs)
{
	collect_string(rs, name);
	if (functions) {
		// :: prefix makes this call global func, not recurse
		::collect_resources(rs, *functions);
	}
	collect_resource(rs, *this->protocol);
}

void dpolymorph_stmt::collect_resources(ResourceSet &rs)
{
	collect_modsym(rs, *protocol);
	if (this->morph_stmts) {
		::collect_resources(rs, *morph_stmts);
	}
	if (this->functions) {
		::collect_resources(rs, *functions);
	}

	collect_resource(rs, *this->polymorph);
}

void goto_stmt::generate_code(AsmFunc &f)
{
	asm_jump(f, label.name, new goto_instruction(0));
}

void label_stmt::generate_code(AsmFunc &f)
{
	label_next(f, label.name);
}

void lcontext_stmt::allocate_registers(RegAlloc *r)
{
	r->alloc(*dst);
}

void lcontext_stmt::generate_code(AsmFunc &f)
{
	asm_instruction(f, new lcontext_instruction(*dst, *name.index));
}

void lfunc_stmt::allocate_registers(RegAlloc *r)
{
	r->alloc(*dst);
}

void lfunc_stmt::generate_code(AsmFunc &f)
{
	asm_instruction(f, new lfunc_instruction(*dst, *modsym->index));
}

void lpfunc_stmt::allocate_registers(RegAlloc *r)
{
	r->alloc(*dst);
}

void lpfunc_stmt::generate_code(AsmFunc &f)
{
	instruction *i;
	i = new lpfunc_instruction(*dst, *protocol->index, *function.index);
	asm_instruction(f, i);
}

void newproc_stmt::allocate_registers(RegAlloc *r)
{
	r->alloc(*pid);
	r->alloc(*func);
}

void newproc_stmt::generate_code(AsmFunc &f)
{
	asm_instruction(f, new newproc_instruction(*pid, *func));
}

void recv_stmt::allocate_registers(RegAlloc *r)
{
	r->alloc(*dst);
	r->alloc(*tube);
}

void recv_stmt::generate_code(AsmFunc &f)
{
	asm_instruction(f, new recv_instruction(*dst, *tube));
}

void stracc_stmt::allocate_registers(RegAlloc *r)
{
	r->alloc(*dst);
	r->alloc(*src);
}

void stracc_stmt::generate_code(AsmFunc &f)
{
	asm_instruction(f, new stracc_instruction(*dst, *src));
}

void unimorph_stmt::allocate_registers(RegAlloc *r)
{
	r->alloc(*pfreg);
	r->alloc(*valreg);
}

void unimorph_stmt::generate_code(AsmFunc &f)
{
	asm_instruction(f, new unimorph_instruction(*pfreg, *valreg));
}

void wait_stmt::allocate_registers(RegAlloc *r)
{
	r->alloc(*reg);
}

void wait_stmt::generate_code(AsmFunc &f)
{
	asm_instruction(f, new wait_instruction(*reg));
}


struct LoadTypeStatement
{
	LoadTypeStatement(uint16_t dstreg, const std::string &modname
			, const std::string &type)
		: dst(dstreg)
		, modname(modname)
		, type(type)
	{}
	uint16_t dst;
	std::string modname;
	std::string type;

	void pretty(ostream &out) const
	{
		out << "ltype " << dst <<" \""<< modname << "\""
			<<" \""<< type << "\"";
	}
};

struct LoadObjStatement
{
	LoadObjStatement(const std::string &modname)
		: modname(modname)
	{}
	std::string modname;

	void pretty(ostream &out) const
	{
		out << "lobj \""<< modname << "\"";
	}
};

void ref_stmt::allocate_registers(RegAlloc *rc)
{
	rc->alloc(*dst);
	rc->alloc(*src);
}

void ref_stmt::generate_code(AsmFunc &f)
{
	asm_instruction(f, new ref_instruction(*dst, *src));
}

void return_stmt::generate_code(AsmFunc &f)
{
	asm_instruction(f, new return_instruction);
}

struct StupleStatement
{
	StupleStatement(uint16_t tup, uint16_t idx, uint16_t data)
		: tup(tup)
		, idx(idx)
		, data(data)
	{}

	uint16_t tup;
	uint16_t idx;
	uint16_t data;

	void pretty(ostream &out) const
	{
		out << "stuple " << tup <<" "<< idx <<" "<< data;
	}
};


inline bool valid_register(int32_t r)
{
	// must be a valid number in the uint16_t range
	int32_t rcmp((uint16_t) r);
	return r == rcmp;
}

void print_function(ostream &out, const AsmFunc &func)
{
	out << func.name.value << " -> " << func.stmts->size()
		<< " statements";
}

void print_statements(ostream &out, const Stmt::List &stmts)
{
	out << stmts.size() << " functions\n";
}

instruction * asm_instruction(const Stmt &s)
{
	instruction *i = 0;
	jump_instruction *j = 0;
	const ClistStatement *clist;
	const ConsStatement *cons;
	const LoadTypeStatement *ltype;
	const LoadObjStatement *lobj;
	const StupleStatement *stup;
	uint16_t modsym;
	if (clist = dynamic_cast<const ClistStatement*>(&s)) {
		i = new clist_instruction(clist->dst);
	} else if (cons = dynamic_cast<const ConsStatement*>(&s)) {
		i = new cons_instruction(cons->head, cons->item);
	} else if (ltype = dynamic_cast<const LoadTypeStatement*>(&s)) {
		uint16_t modstr(0); // add_string(acc, ltype->modname));
		uint16_t typestr(0); // add_string(acc, ltype->type));
		i = new loadtype_instruction(ltype->dst, modstr, typestr);
	} else if (lobj = dynamic_cast<const LoadObjStatement*>(&s)) {
		uint16_t modstr(0); // add_string(acc, lobj->modname));
		i = new loadobj_instruction(modstr);
	} else if (stup = dynamic_cast<const StupleStatement*>(&s)) {
		i = new stuple_instruction(stup->tup, stup->idx, stup->data);
	}
	if (!i) {
		cerr << "null instruction for: ";
		s.pretty(cerr);
		cerr << endl;
		return NULL;
	}
	return i;
}

Stmt::List * parse(istream &in)
{
	string s;
	int toktype;
	void *parser = ParseAlloc(malloc);

	while (! in.eof()) {
		getline(in, s);
		yy_scan_string(s.c_str());
		while (toktype=yylex()) {
			const Token *tok = lexval;
			lexval = NULL;
			Parse(parser, toktype, tok);
			Token::s_column += yyleng;
		}
		++Token::s_lineno;
		Token::s_column = 1;
	}

	Parse(parser, 0, NULL);
	ParseFree(parser, free);

	Stmt::List *result = parsed_stmts;
	parsed_stmts = NULL;
	return result;
}

void collect_resources(ResourceSet &rs, Stmt::List &stmts)
{
	Stmt::List::iterator it(stmts.begin());
	for (; it!=stmts.end(); ++it) {
		(*it)->collect_resources(rs);
	}
}

void index_resources(ResourceSet &rs)
{
	uint16_t index(1);
	ResourceSet::iterator it(rs.begin());
	for (; it!=rs.end(); ++it) {
		it->second = index++;
	}
}

void print_resources(const ResourceSet &rs)
{
	cout << "collected " << rs.size() << " resources\n";

	ResourceSet::const_iterator it(rs.begin());
	for (; it!=rs.end(); ++it) {
		cout << it->second << "\t";
		it->first->pretty(cout);
		cout << endl;
	}
}

void generate_codeblock(AsmFunc &func, const Stmt::List &stmts)
{
	Stmt::List::const_iterator it(stmts.begin());
	for (; it!=stmts.end(); ++it) {
		(*it)->generate_code(func);
	}
}

void generate_code(ResourceSet &rs)
{
	ResourceSet::iterator it(rs.begin());
	AsmFunc *func;
	for (; it!=rs.end(); ++it) {
		if (it->first->type != RESOURCE_FUNCTION) {
			// no code to generate for non-functions
			continue;
		}
		func = static_cast< AsmFunc * >(it->first);
		if (!func->stmts) {
			continue;
		}
		generate_codeblock(*func, *func->stmts);
	}

	measure_jumps(rs);
}

int main(int argc, const char **argv)
{
	istream *in(&cin);
	ifstream fin;
	ofstream out;
	string library_name;

	if (argc >= 2) {
		string input_name(argv[1]);
		int ext_index(input_name.rfind('.'));
		string base_name(input_name.substr(0, ext_index));
		string output_name(base_name +".qb");
		cout << "output_name = " << output_name << endl;
		fin.open(input_name.c_str());
		if (!fin) {
			cerr << "error opening file: " << input_name << endl;
			return -1;
		}
		in = &fin;
		out.open(output_name.c_str(), ios::binary | ios::out);

		library_name = base_name;
	} else {
		out.open("a.qb", ios::binary | ios::out);
	}

	init_instruction_sizes();
	init_writers();
	init_constant_registers();

	ObjectBuilder obj;
	obj.header.qbrt_version = 13;
	obj.header.flags.f.application = 1;
	strcpy(obj.header.library_name, library_name.c_str());
	obj.header.library_version = 22;
	strcpy(obj.header.library_iteration, "bbd");

	cout << "---\n";
	Stmt::List *stmts = parse(*in);
	cout << "---\n";
	print_statements(cout, *stmts);
	cout << "---\n";
	set_function_context(*stmts, FCT_TRADITIONAL, NULL);
	allocate_registers(*stmts, NULL);
	cout << "---\n";
	collect_resources(obj.rs, *stmts);
	index_resources(obj.rs);
	print_resources(obj.rs);
	cout << "---\n";

	generate_code(obj.rs);

	write_object(out, obj);
	out.close();

	return 0;
}

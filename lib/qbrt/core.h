#ifndef QBRT_CORE_H
#define QBRT_CORE_H

#include <stdint.h>
#include <map>
#include <string>
#include <vector>


// TYPE DECLARATIONS

typedef uint16_t reg_t;
typedef uint64_t TaskID;
struct qbrt_value;
struct qbrt_value_index;
struct function_value;
struct Worker;
struct Type;
struct List;
struct Map;
struct Vector;
struct Stream;
struct Tuple;
struct Promise;
struct Failure;
struct Construct;
struct OpContext;
typedef void (*c_function)(OpContext &, qbrt_value &out);


// VALUE TYPES

#define VT_VOID		0x00
#define VT_KIND		0x01
#define VT_BSTRING	0x02
#define VT_FUNCTION	0x03
#define VT_BOOL		0x04
#define VT_FLOAT	0x05
#define VT_REF		0x06
#define VT_TUPLE	0x07
#define VT_LIST		0x09
#define VT_MAP		0x0a
#define VT_VECTOR	0x0b
#define VT_CONSTRUCT	0x0c
#define VT_INT		0x0d
#define VT_STREAM	0x0e
#define VT_HASHTAG	0x0f
#define VT_PROMISE	0x10
#define VT_SET		0x11
#define VT_PATTERNVAR	0x12
#define VT_FAILURE	0xff

extern Type TYPE_VOID;
extern Type TYPE_INT;
extern Type TYPE_BSTRING;
extern Type TYPE_BOOL;
extern Type TYPE_FLOAT;
extern Type TYPE_HASHTAG;
extern Type TYPE_REF;
extern Type TYPE_TUPLE;
extern Type TYPE_FUNCTION;
extern Type TYPE_LIST;
extern Type TYPE_MAP;
extern Type TYPE_VECTOR;
extern Type TYPE_STREAM;
extern Type TYPE_KIND;
extern Type TYPE_PROMISE;
extern Type TYPE_PATTERNVAR;
extern Type TYPE_FAILURE;

struct qbrt_value
{
	union {
		bool b;
		int64_t i;
		std::string *str;
		std::string *hashtag;
		function_value *f;
		// binary_value *bin;
		// uint8_t *bin;
		double fp;
		qbrt_value *ref;
		qbrt_value_index *reg;
		const Type *type;
		Construct *cons;
		Tuple *tuple;
		List *list;
		Map *map;
		Vector *vect;
		Stream *stream;
		Promise *promise;
		Failure *failure;
	} data;
	const Type *type;

	operator List & ()
	{
		return *data.list;
	}
	operator const List & () const
	{
		return *data.list;
	}
	operator Vector * ()
	{
		return data.vect;
	}
	operator const Vector * () const
	{
		return data.vect;
	}

	qbrt_value()
	: type(&TYPE_VOID)
	{
		data.str = NULL;
	}
	static void set_void(qbrt_value &);
	static void b(qbrt_value &v, bool b)
	{
		v.type = &TYPE_BOOL;
		v.data.b = b;
	}
	static void i(qbrt_value &v, int64_t i)
	{
		v.type = &TYPE_INT;
		v.data.i = i;
	}
	static void fp(qbrt_value &v, double f)
	{
		v.type = &TYPE_FLOAT;
		v.data.fp = f;
	}
	static void str(qbrt_value &v, const std::string &s)
	{
		v.type = &TYPE_BSTRING;
		v.data.str = new std::string(s);
	}
	static void hashtag(qbrt_value &v, const std::string &h)
	{
		v.type = &TYPE_HASHTAG;
		v.data.hashtag = new std::string(h);
	}
	static void f(qbrt_value &v, function_value *f)
	{
		v.type = &TYPE_FUNCTION;
		v.data.f = f;
	}
	static void ref(qbrt_value &, qbrt_value &ref);
	static void typ(qbrt_value &v, const Type *t)
	{
		set_void(v);
		v.type = &TYPE_KIND;
		v.data.type = t;
	}
	static void list(qbrt_value &v, List *l)
	{
		set_void(v);
		v.type = &TYPE_LIST;
		v.data.list = l;
	}
	static void construct(qbrt_value &v, const Type *t, Construct *cons)
	{
		v.type = t;
		v.data.cons = cons;
	}
	static void patternvar(qbrt_value &v)
	{
		v.type = &TYPE_PATTERNVAR;
		v.data.reg = NULL;
	}
	static void promise(qbrt_value &v, Promise *p)
	{
		v.type = &TYPE_PROMISE;
		v.data.promise = p;
	}
	static void map(qbrt_value &v, Map *m)
	{
		set_void(v);
		v.type = &TYPE_MAP;
		v.data.map = m;
	}
	static void tuple(qbrt_value &v, Tuple *tup)
	{
		v.type = &TYPE_TUPLE;
		v.data.tuple = tup;
	}
	static void vect(qbrt_value &dst, Vector *v)
	{
		set_void(dst);
		dst.type = &TYPE_VECTOR;
		dst.data.vect = v;
	}
	static void stream(qbrt_value &dst, Stream *s)
	{
		set_void(dst);
		dst.type = &TYPE_STREAM;
		dst.data.stream = s;
	}
	static void fail(qbrt_value &dst, Failure *f)
	{
		set_void(dst);
		dst.type = &TYPE_FAILURE;
		dst.data.failure = f;
	}
	static void copy(qbrt_value &dst, const qbrt_value &src);
	static inline qbrt_value * dup(const qbrt_value &src)
	{
		qbrt_value *dst = new qbrt_value();
		copy(*dst, src);
		return dst;
	}
	static bool is_value_index(const qbrt_value &);
	static bool failed(const qbrt_value &);
	/**
	 * Get the index of the named field for this value
	 *
	 * Return a negative number if the type does not support
	 * field names or the field name does not exist
	 */
	int16_t get_field_index(const std::string &fldname) const;

	static void append_type(std::ostringstream &, const qbrt_value &);

	~qbrt_value() {}
};

struct qbrt_value_index
{
	virtual uint8_t num_values() const = 0;
	virtual qbrt_value & value(uint8_t) = 0;
	virtual const qbrt_value & value(uint8_t) const = 0;
};

int qbrt_compare(const qbrt_value &, const qbrt_value &);

static inline const Type & value_type(const qbrt_value &v)
{
	return *v.type;
}


// OP CODES

#define OP_NOOP		0x00
#define OP_CALL		0x01
#define OP_TAIL_CALL	0x02
#define OP_RETURN	0x03
#define OP_REF		0x04
#define OP_COPY		0x05
#define OP_MOVE		0x06
#define OP_CONSTI	0x08
#define OP_CONSTS	0x09
#define OP_LFUNC	0x0a
#define OP_LOADOBJ	0x0b
#define OP_LOADTYPE	0x0c
#define OP_LCONTEXT	0x0f
#define OP_LONGBR
#define OP_GOTO		0x11
#define OP_IF		0x12
#define OP_IFNOT	0x13
#define OP_IFEQ		0x14
#define OP_IFNOTEQ	0x15
#define OP_IFLT		0x16
#define OP_IFLTEQ	0x17
#define OP_IFGT		0x18
#define OP_IFGTEQ	0x19
#define OP_IFFAIL	0x1a
#define OP_IFNOTFAIL	0x1b
#define OP_EQ
#define OP_LT
#define OP_LE
#define OP_NOT
#define OP_IADD		0x30
#define OP_ISUB		0x31
#define OP_IMULT	0x32
#define OP_IDIV		0x33
#define OP_CTUPLE	0x40
#define OP_STUPLE	0x41
#define OP_CLIST	0x42
#define OP_CONS		0x43
#define OP_CONSTHASH	0x44
#define OP_STRACC	0x46
#define OP_CFAILURE	0x47
#define OP_CALL_LAZY	0x48
#define OP_CALL_REMOTE	0x49
#define OP_FORK		0x4a
#define OP_WAIT		0x4b
#define OP_NEWPROC	0x4c
#define OP_RECV		0x4d
#define OP_MATCH	0x4e
#define OP_LCONSTRUCT	0x4f
#define OP_PATTERNVAR	0x50
#define OP_MATCHARGS	0x51
#define OP_FIELDGET	0x52
#define OP_FIELDSET	0x53
#define NUM_OP_CODES	0x100


// CONSTRUCT REGISTERS
#define PRIMARY_REG(r)		((uint16_t) (0x80 | (r & 0x7f)))
#define SECONDARY_REG(p,s)	((uint16_t) (((p & 0x7f) << 8) | (s & 0x7f)))
#define CONST_REG(id)		((uint16_t) (0x8000 | (0x3fff & id)))
#define SPECIAL_REG(id)		((uint16_t) (0xc000 | (0x3fff & id)))

// REGISTER QUERIES
#define REG_IS_PRIMARY(r)	((r & 0x8080) == 0x0080)
#define REG_IS_SECONDARY(r)	((r & 0x8080) == 0x0000)
#define REG_IS_CONST(r)		((r & 0xc000) == 0x8000)
#define REG_IS_SPECIAL(r)	((r & 0xc000) == 0xc000)

#define REG_EXTRACT_PRIMARY(r)		(r & 0x007f)
#define REG_EXTRACT_SECONDARY1(r)	((r >> 8) & 0x007f)
#define REG_EXTRACT_SECONDARY2(r)	(r & 0x007f)
#define REG_EXTRACT_CONST(r)		(r & 0x3fff)
#define REG_EXTRACT_SPECIAL(r)		(r & 0x3fff)

#define REG_RESULT	0x000
#define REG_PID		0x001

#define SPECIAL_REG_RESULT	(SPECIAL_REG(REG_RESULT))
#define SPECIAL_REG_PID		(SPECIAL_REG(REG_PID))

#define REG_VOID	0x000 // void
#define REG_FALSE	0x010 // false
#define REG_TRUE	0x011 // true
#define REG_FZERO	0x012 // 0.0
#define REG_EMPTYSTR	0x013 // ""
#define REG_NEWLINE	0x014 // "\n"
#define REG_EMPTYLIST	0x015 // []
#define REG_EMPTYVECT	0x016 // []
#define REG_CINT(i)	(0x03f & i) // 0x0 - 0xf
#define CONST_REG_COUNT	0x040

#define CONST_REG_VOID	(CONST_REG(REG_VOID))
#define CONST_REG_FALSE	(CONST_REG(REG_FALSE))
#define CONST_REG_TRUE	(CONST_REG(REG_TRUE))


// register types
// 0b0x/1 -> primary 128
// 0b0x/0 -> secondary registers
// 0b10/x -> const N
// 0b11/x -> special N


std::string pretty_reg(uint16_t);


// CODE

struct instruction
{
	inline const uint8_t & opcode() const
	{
		return *(const uint8_t *) this;
	}
};

struct jump_instruction
: public instruction
{
	int16_t jump() const { return *(int16_t *) (((char *) this) + 1); }
	void setjump(int16_t jmp)
	{
		(*(int16_t *) (((char *) this) + 1)) = jmp;
	}
};


extern uint8_t INSTRUCTION_SIZE[NUM_OP_CODES];
void init_instruction_sizes();

uint8_t isize(uint8_t opcode);

static inline uint8_t isize(const instruction &i)
{
	return isize(i.opcode());
}

static inline void endian_fix(uint16_t &i)
{
	uint8_t *f = (uint8_t *) &i;
	uint8_t tmp = f[0];
	f[0] = f[1];
	f[1] = tmp;
}

static inline uint16_t endian_swap(uint16_t i)
{
	uint8_t *f = (uint8_t *) &i;
	uint8_t tmp = f[0];
	f[0] = f[1];
	f[1] = tmp;
	return i;
}


struct Death {
	friend void operator & (std::ostream &, const Death &);
};
extern Death DIE;

#endif

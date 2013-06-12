#ifndef QBRT_TOKEN_H
#define QBRT_TOKEN_H

#include <string>
#include <iostream>
#include <sstream>
#include <stdint.h>
#include <list>

typedef size_t yy_size_t;
extern int yy_start;
extern yy_size_t yyleng;


struct FileLocation
{
	int lineno;
	int column;

	FileLocation()
	: lineno(0)
	, column(0)
	{}

	FileLocation(int line, int col)
	: lineno(line)
	, column(col)
	{}

	friend std::ostream & operator << (std::ostream &out, FileLocation loc)
	{
		out << loc.lineno <<','<< loc.column;
		return out;
	}
};

struct FileSpan
{
	FileLocation span_front;
	FileLocation span_back;

	FileSpan() {}
	FileSpan(int lineno, int column_front, int length)
	: span_front(lineno, column_front)
	, span_back(lineno, column_front + length - 1)
	{}

	void set_span(FileLocation start, FileLocation end)
	{
		span_front = start;
		span_back = end;
	}
	void copy_span(const FileSpan &s)
		{ set_span(s.span_front, s.span_back); }
	void merge_span(const FileSpan &f, const FileSpan &b)
		{ set_span(f.span_front, b.span_back); }

	std::string span_str() const
	{
		std::ostringstream out;
		out << span_front << "-" << span_back;
		return out.str();
	}
};

struct Token
: public FileSpan
{
	int type;
	std::string text;

	static int s_lineno;
	static int s_column;

	Token()
	: FileSpan(s_lineno, s_column, yyleng)
	, type(0)
	{}

	Token(int typ)
	: FileSpan(s_lineno, s_column, yyleng)
	, type(typ)
	, text()
	{}

	Token(int typ, const std::string &txt)
	: FileSpan(s_lineno, s_column, yyleng)
	, type(typ)
	, text(txt)
	{}

	std::string label() const
	{
		// strip off the leading #
		return text.substr(1);
	}

	std::string strval() const
	{
		return text; // .substr(1, text.length() - 2);
	}

	int64_t intval() const
	{
		int64_t i;
		std::istringstream s(text);
		s >> i;
		return i;
	}

	bool boolval() const
	{
		bool b;
		std::istringstream s(text);
		s >> b;
		return b;
	}


	typedef std::list< const Token * > List;
};

std::ostream & operator << (std::ostream &, const Token &);


extern Token *lexval;

typedef struct yy_buffer_state *YY_BUFFER_STATE;
YY_BUFFER_STATE yy_scan_string (const char *);
int yylex();

#define ParseTOKENTYPE const Token *
void *ParseAlloc(void *(*mallocProc)(size_t));
void Parse(
  void *yyp,                   /* The parser */
  int yymajor,                 /* The major token code number */
  ParseTOKENTYPE yyminor       /* The value for the token */
);
void ParseFree(
  void *p,                    /* The parser to be deleted */
  void (*freeProc)(void*)     /* Function used to reclaim memory */
);
#undef ParseTOKENTYPE

#endif

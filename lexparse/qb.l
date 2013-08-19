
%top {
#include "qbtoken.h"
#include "qbparse.h"
#include <iostream>
#include <sstream>

Token *lexval;
int32_t Token::s_lineno = 1;
int32_t Token::s_column = 1;

#define lex_token(t) do { \
	lexval = new Token(t, yytext); \
	return t; \
} while (0)

using namespace std;

ostringstream g_strlit;
}

DIGIT	[0-9]
INT	\-?{DIGIT}+
PARAM	%[0-9]+
REG	\$[0-9]+
REGEXT	[0-9]+
LABEL	@[a-zA-Z0-9_]+
HASHTAG	#[a-zA-Z0-9_]+
STRC	(\\.|[^\"])
ID	[a-z_][a-z_0-9]*
MODNAME	({ID}\/)*
TYPENAME	[A-Z][a-zA-Z0-9_]*
TYPEVAR	\*[A-Z][a-zA-Z0-9_]*

%x ARGS
%x STR
%x COMMENT

%option noyywrap
%option outfile="lib/qblex.c"

%%

<ARGS>"\""	{
	g_strlit.str("");
	++Token::s_column;
	BEGIN(STR);
}

<ARGS>{PARAM}		{
		lex_token(TOKEN_PARAM);
	}
<ARGS>{REG}		{
		lex_token(TOKEN_REG);
	}
<ARGS>"."{REGEXT}	{
		lex_token(TOKEN_REGEXT);
	}

<ARGS>"\\result"	{
		lex_token(TOKEN_RESULT);
	}
<ARGS>"\\void"		{
		lex_token(TOKEN_VOID);
	}
<ARGS>"\\pid"	{ lex_token(TOKEN_PID); }

<ARGS>{LABEL}		{
		lex_token(TOKEN_LABEL);
	}

<ARGS>{HASHTAG}	{
		lex_token(TOKEN_HASHTAG);
	}

<ARGS>{INT}	{
		lex_token(TOKEN_INT);
	}

"bind"		{ BEGIN(ARGS); lex_token(TOKEN_BIND); }
"bindtype"	{ BEGIN(ARGS); lex_token(TOKEN_BINDTYPE); }
"call"		{ BEGIN(ARGS); lex_token(TOKEN_CALL); }
"cfailure"	{ BEGIN(ARGS); lex_token(TOKEN_CFAILURE); }
"const"		{ BEGIN(ARGS); lex_token(TOKEN_CONST); }
"construct"	{ BEGIN(ARGS); lex_token(TOKEN_CONSTRUCT); }
"copy"		{ BEGIN(ARGS); lex_token(TOKEN_COPY); }
"ctuple"	{ BEGIN(ARGS); lex_token(TOKEN_CTUPLE); }
"datatype"	{ BEGIN(ARGS); lex_token(TOKEN_DATATYPE); }
"dparam"	{ BEGIN(ARGS); lex_token(TOKEN_DPARAM); }
"fork"		{ BEGIN(ARGS); lex_token(TOKEN_FORK); }
"func"		{ BEGIN(ARGS); lex_token(TOKEN_FUNC); }
"goto"		{ BEGIN(ARGS); lex_token(TOKEN_GOTO); }
"if"		{ BEGIN(ARGS); lex_token(TOKEN_IF); }
"ifnot"		{ BEGIN(ARGS); lex_token(TOKEN_IFNOT); }
"ifeq"		{ BEGIN(ARGS); lex_token(TOKEN_IFEQ); }
"ifnoteq"	{ BEGIN(ARGS); lex_token(TOKEN_IFNOTEQ); }
"iffail"	{ BEGIN(ARGS); lex_token(TOKEN_IFFAIL); }
"ifnotfail"	{ BEGIN(ARGS); lex_token(TOKEN_IFNOTFAIL); }
"iadd"		{ BEGIN(ARGS); lex_token(TOKEN_IADD); }
"idiv"		{ BEGIN(ARGS); lex_token(TOKEN_IDIV); }
"imult"		{ BEGIN(ARGS); lex_token(TOKEN_IMULT); }
"isub"		{ BEGIN(ARGS); lex_token(TOKEN_ISUB); }
"lcontext"	{ BEGIN(ARGS); lex_token(TOKEN_LCONTEXT); }
"lconstruct"	{ BEGIN(ARGS); lex_token(TOKEN_LCONSTRUCT); }
"lfunc"		{ BEGIN(ARGS); lex_token(TOKEN_LFUNC); }
"match"		{ BEGIN(ARGS); lex_token(TOKEN_MATCH); }
"matchargs"	{ BEGIN(ARGS); lex_token(TOKEN_MATCHARGS); }
"module"	{ BEGIN(ARGS); lex_token(TOKEN_MODULE); }
"newproc"	{ BEGIN(ARGS); lex_token(TOKEN_NEWPROC); }
"patternvar"	{ BEGIN(ARGS); lex_token(TOKEN_PATTERNVAR); }
"protocol"	{ BEGIN(ARGS); lex_token(TOKEN_PROTOCOL); }
"abstract"	{ BEGIN(ARGS); lex_token(TOKEN_ABSTRACT); }
"recv"		{ BEGIN(ARGS); lex_token(TOKEN_RECV); }
"stracc"	{ BEGIN(ARGS); lex_token(TOKEN_STRACC); }
"wait"		{ BEGIN(ARGS); lex_token(TOKEN_WAIT); }
"noop"		{
		printf("noop token\n");
		lex_token(TOKEN_NOOP);
	}
"ref"		{ BEGIN(ARGS); lex_token(TOKEN_REF); }
"return"	{ lex_token(TOKEN_RETURN); }


"end."		{ lex_token(TOKEN_END); }
{LABEL}		{ lex_token(TOKEN_LABEL); }
"##"	{
	Token::s_column += yyleng; // start the comment
	BEGIN(COMMENT);
}
[ \t]	{
	++Token::s_column; // eat whitespace
}

<ARGS>".\/"		{
	lex_token(TOKEN_CURRENTMOD);
}
<ARGS>{MODNAME}	{
		lex_token(TOKEN_MODNAME);
	}
<ARGS>{ID}	{
		lex_token(TOKEN_ID);
	}
<ARGS>{TYPENAME}	{
		lex_token(TOKEN_TYPENAME);
	}
<ARGS>{TYPEVAR}	{
		lex_token(TOKEN_TYPEVAR);
	}


<ARGS>"("	{ lex_token(TOKEN_LPAREN); }
<ARGS>","	{ lex_token(TOKEN_COMMA); }
<ARGS>")"	{ lex_token(TOKEN_RPAREN); }
<ARGS>[ \t]	{
	/* eat whitespace */
	++Token::s_column;
}
<ARGS>"##"	{
	Token::s_column += yyleng; // start an end of line comment
	BEGIN(COMMENT);
}

<ARGS>.	{
	printf("Unrecognized arg character:\t'%s' %d @ ", yytext, *yytext);
	printf("%d,%d\n", Token::s_lineno, Token::s_column);
	exit(1);
}

.	{
	printf("Unrecognized keyword character:\t'%s' %d @ ", yytext, *yytext);
	printf("%d,%d\n", Token::s_lineno, Token::s_column);
	exit(1);
}


<STR>"\""	{
	lexval = new Token(TOKEN_STR, g_strlit.str());
	++Token::s_column;
	BEGIN(INITIAL);
	return TOKEN_STR;
}
<STR>"\\n"	{
	g_strlit << "\n";
	Token::s_column += 2;
}
<STR>"\\\\"	{
	g_strlit << "\\";
	Token::s_column += 2;
}
<STR>[^\"\\]+ {
	g_strlit << yytext;
	Token::s_column += yyleng;
}

<STR>.	{
	cerr << "Unrecognized string char:\t" << yytext << endl;
	exit(1);
}

<COMMENT>.*	{
	Token::s_column += yyleng;
}

%%

int not_main()
{
	int yt;

	while ((yt = yylex()) != 0) {
		std::cout << "yylex() -> " << lexval->type <<'/'<<
			lexval->text << std::endl;
	}
	return 0;
}

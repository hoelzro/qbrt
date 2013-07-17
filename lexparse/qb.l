
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
UNOP	[a-z_]+
STRC	(\\.|[^\"])

%x STR

%option noyywrap
%option outfile="lib/qblex.c"

%%

"\""	{
	g_strlit.str("");
	++Token::s_column;
	BEGIN(STR);
}

{PARAM}		{
		lex_token(TOKEN_PARAM);
	}
{REG}		{
		lex_token(TOKEN_REG);
	}
"."{REGEXT}	{
		lex_token(TOKEN_REGEXT);
	}

"result"	{
		lex_token(TOKEN_RESULT);
	}
"void"		{
		lex_token(TOKEN_VOID);
	}
"pid"	{ lex_token(TOKEN_PID); }

{LABEL}		{
		lex_token(TOKEN_LABEL);
	}

{HASHTAG}	{
		lex_token(TOKEN_HASHTAG);
	}

{INT}	{
		lex_token(TOKEN_INT);
	}

"brf"		{ lex_token(TOKEN_BRF); }
"brt"		{ lex_token(TOKEN_BRT); }
"brne"		{ lex_token(TOKEN_BRNE); }
"brfail"	{ lex_token(TOKEN_BRFAIL); }
"brnfail"	{ lex_token(TOKEN_BRNFAIL); }
"call"		{ lex_token(TOKEN_CALL); }
"cfailure"	{ lex_token(TOKEN_CFAILURE); }
"const"		{ lex_token(TOKEN_CONST); }
"copy"		{ lex_token(TOKEN_COPY); }
"dfunc"		{ lex_token(TOKEN_DFUNC); }
"dprotocol"	{ lex_token(TOKEN_DPROTOCOL); }
"dpolymorph"	{ lex_token(TOKEN_DPOLYMORPH); }
"fork"		{ lex_token(TOKEN_FORK); }
"morphtype"	{ lex_token(TOKEN_MORPHTYPE); }
"goto"		{ lex_token(TOKEN_GOTO); }
"iadd"		{ lex_token(TOKEN_IADD); }
"idiv"		{ lex_token(TOKEN_IDIV); }
"imult"		{ lex_token(TOKEN_IMULT); }
"isub"		{ lex_token(TOKEN_ISUB); }
"lcontext"	{ lex_token(TOKEN_LCONTEXT); }
"lfunc"		{ lex_token(TOKEN_LFUNC); }
"lpfunc"	{ lex_token(TOKEN_LPFUNC); }
"newproc"	{ lex_token(TOKEN_NEWPROC); }
"recv"		{ lex_token(TOKEN_RECV); }
"stracc"	{ lex_token(TOKEN_STRACC); }
"unimorph"	{ lex_token(TOKEN_UNIMORPH); }
"wait"		{ lex_token(TOKEN_WAIT); }
"noop"		{
		printf("noop token\n");
		lex_token(TOKEN_NOOP);
	}
"ref"		{ lex_token(TOKEN_REF); }
"return"	{ lex_token(TOKEN_RETURN); }


"end."		{ lex_token(TOKEN_END); }
".protocol"	{ lex_token(TOKEN_ENDPROTOCOL); }
".polymorph"	{ lex_token(TOKEN_ENDPOLYMORPH); }

[ \t]	{
	/* eat whitespace */
	++Token::s_column;
}

.	{
	printf("Unrecognized character:\t'%s' %d @ ", yytext, *yytext);
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

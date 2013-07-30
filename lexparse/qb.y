%token_type {const Token *}
%token_prefix TOKEN_
%type program {Stmt::List *}
%type top_list {Stmt::List *}
%type top_stmt {Stmt *}
%type modsym {AsmModSym *}
%type modtype {AsmModSym *}
%type reg {AsmReg *}
%type stmt {Stmt *}
%type func_block {dfunc_stmt *}
%type dfunc_stmt {dfunc_stmt *}
%type dparam_block {dparam_stmt::List *}
%type dparam_stmt {dparam_stmt *}
%type fork_block {fork_stmt *}
%type fork_stmt {fork_stmt *}
%type block {Stmt::List *}
%type sub_block {Stmt::List *}
%type func_list {Stmt::List *}
%type protocol_block {protocol_stmt *}
%type protocol_stmt {protocol_stmt *}
%type protoabstract_block {dfunc_stmt *}
%type protoabstract_stmt {dfunc_stmt *}
%type protofunc_block {dfunc_stmt *}
%type protofunc_stmt {dfunc_stmt *}
%type protofunc_list {Stmt::List *}
%type bind_block {bind_stmt *}
%type bind_stmt {bind_stmt *}
%type bindtype_block {list< bindtype_stmt * > *}
%type bindtype_stmt {bindtype_stmt *}
%type typename_list {list< AsmString * > *}


%include {
#include <iostream>
#include <cassert>
#include <cstdlib>
#include "qbparse.h"
#include "qbtoken.h"
#include "asm.h"

using namespace std;

extern string g_current_module;
}

%syntax_error {
	cout << "Qbrt syntax error. wtf? @ ";
	cout << Token::s_lineno << "," << Token::s_column << endl;
}

%parse_failure {
	cout << "Parse failed." << endl;
}

program ::= top_list(B). {
	parsed_stmts = B;
}

top_list(A) ::= top_list(B) top_stmt(C). {
	A = B;
	A->push_back(C);
}
top_list(A) ::= top_stmt(B). {
	A = new Stmt::List();
	A->push_back(B);
}

top_stmt(A) ::= MODULE MODNAME. {
	cerr << "whoops, module declarations are not implemented yet\n";
	A = NULL;
}
top_stmt(A) ::= func_block(B). { A = B; }
top_stmt(A) ::= protocol_block(B). { A = B; }
top_stmt(A) ::= bind_block(B). { A = B; }

block(A) ::= sub_block(B) END. {
	A = B;
}
block(A) ::= END. {
	A = NULL;
}
sub_block(A) ::= sub_block(B) stmt(C). {
	A = B;
	A->push_back(C);
}
sub_block(A) ::= stmt(B). {
	A = new Stmt::List();
	A->push_back(B);
}

func_block(A) ::= dfunc_stmt(B) dparam_block(D) block(C). {
	B->params = D;
	B->code = C;
	A = B;
	A->code->push_back(new return_stmt());
}
dfunc_stmt(A) ::= FUNC ID(B). {
	A = new dfunc_stmt(B->text);
}

dparam_block(A) ::= dparam_block(B) dparam_stmt(C). {
	if (!B) {
		A = new dparam_stmt::List();
	} else {
		A = B;
	}
	A->push_back(C);
}
dparam_block(A) ::= . {
	A = NULL;
}
dparam_stmt(A) ::= DPARAM ID(B) modtype(C). {
	A = new dparam_stmt(B->strval(), C);
}

func_list(A) ::= func_list(B) func_block(C). {
	A = B ? B : new Stmt::List();
	if (C) {
		A->push_back(C);
	} else {
		cerr << "null function block\n";
	}
}
func_list(A) ::= . {
	A = NULL;
}

protocol_stmt(A) ::= PROTOCOL TYPENAME(B) typename_list(C). {
	A = new protocol_stmt(B->text, C);
}
protocol_block(A) ::= protocol_stmt(B) protofunc_list(C) END. {
	A = B;
	A->functions = C;
}

protofunc_list(A) ::= . {
	A = new Stmt::List();
}
protofunc_list(A) ::= protofunc_list(B) protoabstract_block(C). {
	A = B;
	A->push_back(C);
}
protofunc_list(A) ::= protofunc_list(B) protofunc_block(C). {
	A = B;
	A->push_back(C);
}

protoabstract_stmt(A) ::= PROTOABSTRACT TYPENAME(B) ID(C). {
	A = new dfunc_stmt(B->text, C->text, true);
}
protoabstract_block(A) ::= protoabstract_stmt(B) dparam_block(C) END. {
	A = B;
	A->params = C;
}

protofunc_stmt(A) ::= PROTOFUNC TYPENAME(B) ID(C). {
	A = new dfunc_stmt(B->text, C->text, false);
}
protofunc_block(A) ::= protofunc_stmt(B) dparam_block(C) block(D). {
	A = B;
	A->params = C;
	A->code = D;
}

bind_stmt(A) ::= BIND modtype(B). {
	A = new bind_stmt(B);
}
bindtype_stmt(A) ::= BINDTYPE modtype(B). {
	A = new bindtype_stmt(B);
}
bindtype_block(A) ::= bindtype_stmt(B). {
	A = new list< bindtype_stmt * >();
	A->push_back(B);
}
bindtype_block(A) ::= bindtype_block(B) bindtype_stmt(C). {
	A = B;
	A->push_back(C);
}
bind_block(A) ::= bind_stmt(B) bindtype_block(C) func_list(D) END. {
	A = B;
	A->params = C;
	A->functions = D;
}


fork_block(A) ::= fork_stmt(B) block(C). {
	A = B;
	A->code = C;
	A->code->push_back(new return_stmt());
}
fork_stmt(A) ::= FORK reg(B). {
	A = new fork_stmt(B);
}

stmt(A) ::= fork_block(B). {
	A = B;
}
stmt(A) ::= BRF reg(C) LABEL(B). {
	A = new brbool_stmt(false, C, B->label());
}
stmt(A) ::= BRNE reg(B) reg(C) LABEL(D). {
	A = brcmp_stmt::ne(B, C, D->label());
}
stmt(A) ::= BRFAIL reg(B) LABEL(C). {
	A = new brfail_stmt(true, B, C->label());
}
stmt(A) ::= BRNFAIL reg(B) LABEL(C). {
	A = new brfail_stmt(false, B, C->label());
}
stmt(A) ::= BRT reg(C) LABEL(B). {
	A = new brbool_stmt(true, C, B->label());
}
stmt(A) ::= CALL reg(B) reg(C). {
	A = new call_stmt(B, C);
}
stmt(A) ::= CFAILURE HASHTAG(B). {
	A = new cfailure_stmt(B->label());
}
stmt(A) ::= CONST reg(B) INT(C). {
	A = new consti_stmt(B, C->intval());
}
stmt(A) ::= CONST reg(B) STR(C). {
	A = new consts_stmt(B, C->strval());
}
stmt(A) ::= CONST reg(B) HASHTAG(C). {
	A = new consthash_stmt(B, C->label());
}
stmt(A) ::= COPY reg(B) reg(C). {
	A = new copy_stmt(B, C);
}
stmt(A) ::= GOTO LABEL(B). {
	A = new goto_stmt(B->label());
}
stmt(A) ::= IADD reg(B) reg(C) reg(D). {
	A = new binaryop_stmt('+', 'i', B, C, D);
}
stmt(A) ::= IDIV reg(B) reg(C) reg(D). {
	A = new binaryop_stmt('/', 'i', B, C, D);
}
stmt(A) ::= IMULT reg(B) reg(C) reg(D). {
	A = new binaryop_stmt('*', 'i', B, C, D);
}
stmt(A) ::= ISUB reg(B) reg(C) reg(D). {
	A = new binaryop_stmt('-', 'i', B, C, D);
}
stmt(A) ::= LABEL(B). {
	A = new label_stmt(B->label());
}
stmt(A) ::= LCONTEXT reg(B) HASHTAG(C). {
	A = new lcontext_stmt(B, C->label());
}
stmt(A) ::= LFUNC reg(B) modsym(C). {
	A = new lfunc_stmt(B, C);
	C = NULL;
}
stmt(A) ::= LPFUNC reg(B) modsym(C) STR(D). {
	A = new lpfunc_stmt(B, C, D->strval());
	C = NULL;
}
stmt(A) ::= NEWPROC reg(B) reg(C). {
	A = new newproc_stmt(B, C);
}
stmt(A) ::= RECV reg(B) reg(C). {
	A = new recv_stmt(B, C);
}
stmt(A) ::= STRACC reg(B) reg(C). {
	A = new stracc_stmt(B, C);
}
stmt(A) ::= WAIT reg(B). {
	A = new wait_stmt(B);
}
stmt ::= NOOP. { std::cout << "noop\n"; }
stmt(A) ::= REF reg(B) reg(C). {
	A = new ref_stmt(B, C);
}
stmt(A) ::= RETURN. {
	A = new return_stmt();
}

typename_list(A) ::= typename_list(B) TYPENAME(C). {
	A = B;
	A->push_back(new AsmString(C->text));
}
typename_list(A) ::= TYPENAME(B). {
	A = new list< AsmString * >();
	A->push_back(new AsmString(B->text));
}

modtype(A) ::= MODNAME(B) TYPENAME(C). {
	Token *t = const_cast< Token * >(B);
	t->text.erase(t->text.size() - 1, 1);
	A = new AsmModSym(t->text, C->text);
}
modtype(A) ::= CURRENTMOD TYPENAME(C). {
	A = new AsmModSym(g_current_module, C->text);
}

modsym(A) ::= MODNAME(B) ID(C). {
	Token *t = const_cast< Token * >(B);
	t->text.erase(t->text.size() - 1, 1);
	A = new AsmModSym(t->text, C->text);
}
modsym(A) ::= CURRENTMOD ID(C). {
	A = new AsmModSym(g_current_module, C->text);
}

reg(A) ::= REG(B) REGEXT(C). {
	A = AsmReg::parse_extended_reg(B->text, C->text);
	if (A->type != '$') {
		cerr << "secondary register is not $\n";
		exit(1);
	}
}
reg(A) ::= PARAM(B) REGEXT(C). {
	A = AsmReg::parse_extended_arg(B->text, C->text);
	if (A->type != '%') {
		cerr << "secondary arg is not %%\n";
		exit(1);
	}
}
reg(A) ::= REG(B). {
	A = AsmReg::parse_reg(B->text);
	if (A->type != '$') {
		cerr << "primary register is not $\n";
		exit(1);
	}
}
reg(A) ::= PARAM(B). {
	A = AsmReg::parse_arg(B->text);
	if (A->type != '%') {
		cerr << "primary arg is not %: '" << B->text << "'\n";
		exit(1);
	}
}
reg(A) ::= PID. {
	A = AsmReg::create_special(REG_PID);
}
reg(A) ::= RESULT. {
	A = AsmReg::create_result();
}
reg(A) ::= VOID. {
	A = AsmReg::create_void();
}

%token_type {const Token *}
%token_prefix TOKEN_
%type program {Stmt::List *}
%type top_list {Stmt::List *}
%type top_stmt {Stmt *}
%type modsym {AsmModSym *}
%type reg {AsmReg *}
%type stmt {Stmt *}
%type func_block {dfunc_stmt *}
%type dfunc_stmt {dfunc_stmt *}
%type fork_block {fork_stmt *}
%type fork_stmt {fork_stmt *}
%type block {Stmt::List *}
%type sub_block {Stmt::List *}
%type func_list {Stmt::List *}
%type protocol_block {dprotocol_stmt *}
%type dprotocol_stmt {dprotocol_stmt *}
%type polymorph_block {dpolymorph_stmt *}
%type dpolymorph_stmt {dpolymorph_stmt *}
%type morphtype_stmt {morphtype_stmt *}
%type morphtype_list {Stmt::List *}


%include {
#include <iostream>
#include <cassert>
#include <cstdlib>
#include "qbparse.h"
#include "qbtoken.h"
#include "asm.h"

using namespace std;
}

%syntax_error {
	cout << "Qbrt syntax error. wtf? @ ";
	cout << Token::s_lineno << "," << Token::s_column << endl;
}

%parse_failure {
	cout << "Parse failed." << endl;
}

program ::= top_list(B). {
	cout << "program is code\n";
	cout << B->size() << " function declarations\n";
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

top_stmt(A) ::= func_block(B). { A = B; }
top_stmt(A) ::= protocol_block(B). { A = B; }
top_stmt(A) ::= polymorph_block(B). { A = B; }

block(A) ::= sub_block(B) END. {
	A = B;
cerr << "end block\n";
}
block(A) ::= END. {
	A = NULL;
cerr << "empty block\n";
}
sub_block(A) ::= sub_block(B) stmt(C). {
	A = B;
	A->push_back(C);
}
sub_block(A) ::= stmt(B). {
	A = new Stmt::List();
	A->push_back(B);
}

func_block(A) ::= dfunc_stmt(B) block(C). {
	cout << "dfunc_stmt " << B->name.value << "/" << (int) B->arity << endl;
	B->code = C;
	A = B;
}
dfunc_stmt(A) ::= DFUNC STR(B) INT(C). {
	A = new dfunc_stmt(B->strval(), C->intval());
}

func_list(A) ::= func_list(B) func_block(C). {
	A = B ? B : new Stmt::List();
	if (C) {
		C->pretty(cout);
		cout << endl;
		A->push_back(C);
	} else {
		cout << "null function block\n";
	}
}
func_list ::= . {
	cout << "end of function list" << endl;
}
protocol_block(A) ::= dprotocol_stmt(B) func_list(C) ENDPROTOCOL. {
	A = B;
	A->functions = C;
}
dprotocol_stmt(A) ::= DPROTOCOL STR(B) INT(C). {
	A = new dprotocol_stmt(B->strval(), C->intval());
}

morphtype_list(A) ::= morphtype_list(B) morphtype_stmt(C). {
	A = B ? B : new Stmt::List();
	if (C) {
		C->pretty(cout);
		cout << endl;
		A->push_back(C);
	} else {
		cout << "null morphtype block\n";
	}
}
morphtype_list ::= . {
	cout << "end of morphtype list" << endl;
}
morphtype_stmt(A) ::= MORPHTYPE modsym(B). {
	A = new morphtype_stmt(B);
	B = NULL;
}
polymorph_block(A) ::= dpolymorph_stmt(B) morphtype_list(C) func_list(D) ENDPOLYMORPH. {
	A = B;
	A->morph_stmts = C;
	A->functions = D;
}
dpolymorph_stmt(A) ::= DPOLYMORPH modsym(B). {
	A = new dpolymorph_stmt(B);
	B = NULL;
}

fork_block(A) ::= fork_stmt(B) block(C). {
	A = B;
	A->code = C;
cerr << "fork block\n";
}
fork_stmt(A) ::= FORK reg(B). {
	A = new fork_stmt(B);
}

stmt(A) ::= fork_block(B). {
	A = B;
}
stmt(A) ::= ADDI reg(B) reg(C) reg(D). {
	A = new binaryop_stmt('+', 'i', B, C, D);
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
stmt ::= LCONST reg(A) STR(B). {
     std::cout << "lconst " << A <<" " << B->text << endl;
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
stmt(A) ::= STRACC reg(B) reg(C). {
	A = new stracc_stmt(B, C);
}
stmt(A) ::= UNIMORPH reg(B) reg(C). {
	A = new unimorph_stmt(B, C);
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

modsym(A) ::= STR(B) STR(C). {
	A = new AsmModSym(B->strval(), C->strval());
}

reg(A) ::= REG(B) REGEXT(C). {
	A = AsmReg::parse_extended_reg(B->text, C->text);
}
reg(A) ::= PARAM(B) REGEXT(C). {
	A = AsmReg::parse_extended_arg(B->text, C->text);
}
reg(A) ::= REG(B). {
	A = AsmReg::parse_reg(B->text);
}
reg(A) ::= PARAM(B). {
	A = AsmReg::parse_arg(B->text);
}
reg(A) ::= RESULT. {
	A = AsmReg::create_result();
}
reg(A) ::= VOID. {
	A = AsmReg::create_void();
}

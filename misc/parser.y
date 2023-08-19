%{
  #include <queue>
  #include <iostream>
  #include "inc/assembler.h"
  using namespace std;
  // Declare stuff from Flex that Bison needs to know about:
  extern "C" int yylex();

  void yyerror(const char *s);

  queue<char *> symbols;  // for operands of .global, .extern and .word directvies
%}

%union {
  char *sval;
  unsigned ival;
}

%token GLOBAL EXTERN SECTION WORD SKIP END              // directives
%token HALT INT IRET CALL RET                           // control instructions
%token JMP BEQ BNE BGT                                  // jump instructions
%token PUSH POP XCHG LD ST                              // register/memory instructions
%token ADD SUB MUL DIV NOT AND OR XOR SHL SHR           // ALU instructions
%token CSRRD CSRWR                                      // control status register instructions
%token EOL                                              // end of line
%token <ival> GPR                                       // general purpose registers (r0 - r15)
%token <ival> CSR                                       // control and status registers (status, handler, cause)
%token <ival> NUM                                       // integer number
%token <sval> SYM                                       // symbol (string)


%%

input: lines;

lines: line | lines line;

line: EOL | line_content EOL | END { Assembler::_end(); };

line_content: 

  directive
|
  label | label instruction
|
  instruction
;

directive: 

  GLOBAL symbol_list                      {
                                            while (!symbols.empty()) {
                                              Assembler::_global(symbols.front());
                                              symbols.pop();
                                            }
                                          }
| 
  EXTERN symbol_list                      {
                                            while (!symbols.empty()) {
                                              Assembler::_extern(symbols.front());
                                              symbols.pop();
                                            }
                                          }
| 
  SECTION SYM                             { Assembler::_section($2); }
|
  word
| 
  SKIP NUM                                { Assembler::_skip($2); }
;

word:
  WORD SYM                                { Assembler::_word($2); }
|
  WORD NUM                                { Assembler::_word($2); }
|
  word SYM                                { Assembler::_word($2); }
|
  word NUM                                { Assembler::_word($2); }
;

symbol_list: 
  SYM                                     { symbols.push($1); } 
|
  symbol_list ',' SYM                     { symbols.push($3); }
;


label: SYM ':'                            { Assembler::_label($1); };

instruction:
  HALT                                    { Assembler::_halt(); }
|
  INT                                     { Assembler::_int(); }
|
  IRET                                    { Assembler::_iret(); }
|
  call
|
  RET                                     { Assembler::_ret(); }
|
  jmp
|
  beq
|
  bne
|
  bgt
|
  PUSH '%' GPR                            { Assembler::_push($3); }
|
  POP '%' GPR                             { Assembler::_pop($3); }
|
  XCHG '%' GPR ',' '%' GPR                { Assembler::_xchg($3, $6); }
|
  ADD '%' GPR ',' '%' GPR                 { Assembler::_add($3, $6); }
|
  SUB '%' GPR ',' '%' GPR                 { Assembler::_sub($3, $6); }
|
  MUL '%' GPR ',' '%' GPR                 { Assembler::_mul($3, $6); }
|
  DIV '%' GPR ',' '%' GPR                 { Assembler::_div($3, $6); }
|
  NOT '%' GPR                             { Assembler::_not($3); }
|
  AND '%' GPR ',' '%' GPR                 { Assembler::_and($3, $6); }
|
  OR '%' GPR ',' '%' GPR                  { Assembler::_or($3, $6); }
|
  XOR '%' GPR ',' '%' GPR                 { Assembler::_xor($3, $6); }
|
  SHL '%' GPR ',' '%' GPR                 { Assembler::_shl($3, $6); }
|
  SHR '%' GPR ',' '%' GPR                 { Assembler::_shr($3, $6); }
|
  ld
|
  st
|
  CSRRD CSR ',' '%' GPR                   { Assembler::_csrrd($2, $5); }
|
  CSRWR '%' GPR ',' CSR                   { Assembler::_csrwr($3, $5); }
;

call:
  CALL NUM                                { Assembler::_call($2); }
|
  CALL SYM                                { Assembler::_call($2); }
;

jmp:
  JMP NUM                                 { Assembler::_jmp($2); }
|
  JMP SYM                                 { Assembler::_jmp($2); }
;

beq:
  BEQ '%' GPR ',' '%' GPR ',' NUM         { Assembler::_beq($3, $6, $8); }
|
  BEQ '%' GPR ',' '%' GPR ',' SYM         { Assembler::_beq($3, $6, $8); }
;

bne:
  BNE '%' GPR ',' '%' GPR ',' NUM         { Assembler::_bne($3, $6, $8); }
|
  BNE '%' GPR ',' '%' GPR ',' SYM         { Assembler::_bne($3, $6, $8); }
;

bgt:
  BGT '%' GPR ',' '%' GPR ',' NUM         { Assembler::_bgt($3, $6, $8); }
|
  BGT '%' GPR ',' '%' GPR ',' SYM         { Assembler::_bgt($3, $6, $8); }
;

ld:
  LD '$' NUM ',' '%' GPR                  { Assembler::_ldImm($3, $6); }
|
  LD '$' SYM ',' '%' GPR                  { Assembler::_ldImm($3, $6); }
|
  LD NUM ',' '%' GPR                      { Assembler::_ldMemDir($2, $5); }
|
  LD SYM ',' '%' GPR                      { Assembler::_ldMemDir($2, $5); }
|
  LD '%' GPR ',' '%' GPR                  { Assembler::_ldReg($3, $6); }
|
  LD '[' '%' GPR ']' ',' '%' GPR          { Assembler::_ldRegInd($4, $8); }
|
  LD '[' '%' GPR '+' NUM ']' ',' '%' GPR  { Assembler::_ldRegIndOff($4, $6, $10); }
|
  LD '[' '%' GPR '+' SYM ']' ',' '%' GPR  { Assembler::_ldRegIndOff($4, $6, $10); }
;

st:
  ST '%' GPR ',' '$' NUM                  { Assembler::_stImm($3, $6); }
|
  ST '%' GPR ',' '$' SYM                  { Assembler::_stImm($3, $6); }
|
  ST '%' GPR ',' NUM                      { Assembler::_stMemDir($3, $5); }
|
  ST '%' GPR ',' SYM                      { Assembler::_stMemDir($3, $5); }
|
  ST '%' GPR ',' '%' GPR                  { Assembler::_stReg($3, $6); }
|
  ST '%' GPR ',' '[' '%' GPR ']'          { Assembler::_stRegInd($3, $7); }
|
  ST '%' GPR ',' '[' '%' GPR '+' NUM ']'  { Assembler::_stRegIndOff($3, $7, $9); }
|
  ST '%' GPR ',' '[' '%' GPR '+' SYM ']'  { Assembler::_stRegIndOff($3, $7, $9); }
;

%%

void yyerror(const char *s) {
  cout << s << endl;
  exit(-1);
}
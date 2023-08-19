%{
  #include <cstdio>
  #include <iostream>
  #include "parser.tab.h" 
  using namespace std;
  extern "C" int yylex();
%}

%%
.global                 { return GLOBAL; }
.extern                 { return EXTERN; }
.section                { return SECTION; }
.word                   { return WORD; }
.skip                   { return SKIP; }
.end                    { return END; }
halt                    { return HALT; }
int                     { return INT; }       
iret                    { return IRET; }
call                    { return CALL; }
ret                     { return RET; }
jmp                     { return JMP; }
beq                     { return BEQ; }
bne                     { return BNE; }
bgt                     { return BGT; }
push                    { return PUSH; }
pop                     { return POP; }
xchg                    { return XCHG; }
add                     { return ADD; }
sub                     { return SUB; }
mul                     { return MUL; }
div                     { return DIV; }
not                     { return NOT; }
and                     { return AND; }
or                      { return OR; }
xor                     { return XOR; }
shl                     { return SHL; }
shr                     { return SHR; }
ld                      { return LD; }
st                      { return ST; }
csrrd                   { return CSRRD; }
csrwr                   { return CSRWR; }
r[0-9][0-5]?            { yylval.ival = atoi(yytext + 1);     return GPR; }
sp                      { yylval.ival = 14;                   return GPR; }
pc                      { yylval.ival = 15;                   return GPR; }
%status                 { yylval.ival = 0;                    return CSR; }
%handler                { yylval.ival = 1;                    return CSR; }
%cause                  { yylval.ival = 2;                    return CSR; }
-?[0-9]+                { yylval.ival = atoi(yytext);         return NUM; }
0x[0-9a-fA-F]+          { sscanf(yytext, "%x", &yylval.ival); return NUM; }
[a-zA-Z_][a-zA-Z0-9_]*  { yylval.sval = strdup(yytext);       return SYM; }
[%]                     { return '%'; }
[$]                     { return '$'; }
[:]                     { return ':'; }
[\[]                    { return '['; }
[\]]                    { return ']'; }
[+]                     { return '+'; }
[,]                     { return ','; }
[*]                     { return '*'; }
[\n]                    { return EOL; }
[#].*[\n]               { return EOL; }
[ \t]                   ;
.                       ;
%%


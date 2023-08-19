#include "../inc/assembler.h"
#include <iostream>
#include <string>

#define INSTR_SIZE 4
#define D_SIZE 12
const int D_MAX = (1 << D_SIZE - 1) - 1;
const int D_MIN = -(1 << D_SIZE - 1);

// ----------------------------------- REGISTERS ---------------------------------
#define STATUS 0
#define HANDLER 1
#define CAUSE 2
#define ACC 13
#define SP 14
#define PC 15

// -------------------------------- OPERATION CODES ------------------------------

#define HALT_OC   0x0
#define INT_OC    0x1
#define CALL_OC   0x2
#define JMP_OC    0x3
#define XCHG_OC   0x4
#define ARI_OP_OC 0x5
#define LOG_OP_OC 0x6
#define SH_OP_OC  0x7
#define ST_OC     0x8
#define LD_OC     0x9

// ----------------------------------- LOAD MODES ---------------------------------

#define LD_GPR_M1 0x0 //  gpr[A]<=csr[B];
#define LD_GPR_M2 0x1 //  gpr[A]<=gpr[B]+D;
#define LD_GPR_M3 0x2 //  gpr[A]<=mem32[gpr[B]+gpr[C]+D];
#define LD_GPR_M4 0x3 //  gpr[A]<=mem32[gpr[B]]; gpr[B]<=gpr[B]+D;
#define LD_CSR_M1 0x4 //  csr[A]<=gpr[B];
#define LD_CSR_M2 0x5 //  csr[A]<=csr[B]|D;
#define LD_CSR_M3 0x6 //  csr[A]<=mem32[gpr[B]+gpr[C]+D];
#define LD_CSR_M4 0x7 //  csr[A]<=mem32[gpr[B]]; gpr[B]<=gpr[B]+D;

// ---------------------------------- STORE MODES ---------------------------------

#define ST_M1     0x0 //  mem32[gpr[A]+gpr[B]+D]<=gpr[C];
#define ST_M2     0x2 //  mem32[mem32[gpr[A]+gpr[B]+D]]<=gpr[C];
#define ST_M3     0x1 //  gpr[A]<=gpr[A]+D; mem32[gpr[A]]<=gpr[C];

// ----------------------------------- CALL MODES ---------------------------------

#define CALL_M1     0x0 //  push pc; pc<=gpr[A]+gpr[B]+D;
#define CALL_M2     0x1 //  push pc; pc<=mem32[gpr[A]+gpr[B]+D];

// ----------------------------------- JUMP MODES ---------------------------------

#define JMP_M1      0x0 //  pc<=gpr[A]+D;
#define JMP_M2      0x1 //  if (gpr[B] == gpr[C]) pc<=gpr[A]+D;
#define JMP_M3      0x2 //  if (gpr[B] != gpr[C]) pc<=gpr[A]+D;
#define JMP_M4      0x3 //  if (gpr[B] signed > gpr[C]) pc<=gpr[A]+D;
#define JMP_M5      0x8 //  pc<=mem[gpr[A]+D];
#define JMP_M6      0x9 //  if (gpr[B] == gpr[C]) pc<=mem[gpr[A]+D];
#define JMP_M7      0xA //  if (gpr[B] != gpr[C]) pc<=mem[gpr[A]+D];
#define JMP_M8      0xB //  if (gpr[B] signed > gpr[C]) pc<=mem[gpr[A]+D];

// ------------------------------------ ALU MODES ---------------------------------

#define ADD         0x0 //  gpr[A]<=gpr[B] + gpr[C];
#define SUB         0x1 //  gpr[A]<=gpr[B] - gpr[C];
#define MUL         0x2 //  gpr[A]<=gpr[B] * gpr[C];
#define DIV         0x3 //  gpr[A]<=gpr[B] / gpr[C];
#define NOT         0x0 //  gpr[A]<=~gpr[B];
#define AND         0x1 //  gpr[A]<=gpr[B] & gpr[C];
#define OR          0x2  //  gpr[A]<=gpr[B] | gpr[C];
#define XOR         0x3 //  gpr[A]<=gpr[B] ^ gpr[C];
#define SHL         0x0 //  gpr[A]<=gpr[B] << gpr[C];
#define SHR         0x1 //  gpr[A]<=gpr[B] >> gpr[C];

// -------------------------------- STATIC VARIABLES ------------------------------

unsigned Assembler::locationCounter = 0;
bool Assembler::first = true;
int Assembler::currSecId = 0;
string Assembler::currSecName = "";
vector<Assembler::Symbol *> Assembler::symbolTable;
vector<Assembler::Section *> Assembler::sectionTable;
FILE *Assembler::asmFile;
MyElf *Assembler::myElf = new MyElf();

// -------------------------------- HELPER FUNCTIONS ------------------------------

void Assembler::init()
{
  sectionTable.push_back(new Section("UND"));
  myElf->sections.push_back(new MyElf::Section(0, "UND"));
}


void Assembler::createMyElfSymbolTable()
{
  // Insert sections into symbol table
  for (int i = 0; i < sectionTable.size(); i++)
  {
    myElf->symbolTable.push_back(new MyElf::Symbol(sectionTable[i]->name, 0, false, true, i));
  }

  // Insert symbols into symbol table
  for (Symbol *s : symbolTable)
  {
    myElf->symbolTable.push_back(new MyElf::Symbol(s->name, s->value, s->global, false, s->sectionId));
  }
}


void Assembler::setGlobal(char *symbol)
{
  int i = 0;
  for (; i < symbolTable.size(); i++)
  {
    if (symbolTable[i]->name == symbol)
    {
      symbolTable[i]->global = true;
      break;
    }
  }

  if (i == symbolTable.size())
    symbolTable.push_back(new Symbol(symbol, 0, true, 0));
}


void Assembler::newRelocation(int offset, char *symbol, MyElf::RelocationTypes type, int addend)
{
  // Find symbol in symbol table
  int symId = myElf->symbolId(symbol);

  // If symbol doesn't exist, report error and exit
  if (symId == -1)
  {
    cout << "Symbol " << symbol << " (used on byte " << locationCounter << " of section " << currSecId
         << ") is not declared in this file!" << endl;
    exit(-2);
  }

  // If symbol is local (linker won't see it)
  if (!myElf->symbolTable[symId]->isGlobal) {
    addend += myElf->symbolTable[symId]->value;
    symId = myElf->symbolTable[symId]->sectionId;
  }

  // Find current section's relocation table place
  int p = myElf->secRelTabId(currSecId);

  // If section doesn't have relocation table, create it
  if (p == -1)
  {
    MyElf::RelocationTable *relocationTable = new MyElf::RelocationTable(currSecId, currSecName);
    myElf->relocationTables.push_back(relocationTable);
    p = myElf->relocationTables.size() - 1;
  }

  // Create new relocation
  MyElf::Relocation *relocation = new MyElf::Relocation(offset, type, symId, addend);

  // Insert relocation into relocation table for current section
  myElf->relocationTables[p]->relocations.push_back(relocation);
}


char Assembler::getByte(int value, int byteIndex)
{
  return (value >> (byteIndex * 8)) & 0xFF;
}


void Assembler::codeInstruction(char byte1, char byte2, char byte3, char byte4)
{
  myElf->sections[currSecId]->memory.push_back(byte1);
  myElf->sections[currSecId]->memory.push_back(byte2);
  myElf->sections[currSecId]->memory.push_back(byte3);
  myElf->sections[currSecId]->memory.push_back(byte4);
  locationCounter += INSTR_SIZE;
}


void Assembler::literalPoolProcessing(char OC, char M1, char M2, char A1, char A2, char B, char C, int D)
{
  if (!(D > D_MAX) && !(D < D_MIN))
  {
    // Literal can fit in instruction
    codeInstruction((OC << 4) | M1, (A1 << 4) | B, (C << 4) | (getByte(D, 1) & 0x0F), getByte(D, 0));
  }
  else
  {
    // Literal can't fit in insturction, we must put it in literal pool
    myElf->sections[currSecId]->literalPool.push_back(D);

    int offset = myElf->sections[currSecId]->size - locationCounter         // gap to the end of section*
                 + (myElf->sections[currSecId]->literalPool.size() - 1) * 4 // offset in literal pool array
                 - INSTR_SIZE                                               // because pc is pointing to the next instruction
                 + INSTR_SIZE;                                              // because we will add another instruction (to avoid literal pool) at the end of the section

    codeInstruction((OC << 4) | M2, (A2 << 4) | B, (C << 4) | (getByte(offset, 1) & 0x0F), getByte(offset, 0));
  }
}


void Assembler::literalPoolProcessing(char OC, char M, char A, char B, char C, char *D)
{
  // We need to "put" symbol in literal pool and create new relocation for it
  myElf->sections[currSecId]->literalPool.push_back(0);

  int offset = myElf->sections[currSecId]->size - locationCounter         // gap to the end of section*
               + (myElf->sections[currSecId]->literalPool.size() - 1) * 4 // offset in literal pool array
               - INSTR_SIZE                                               // because pc is pointing to the next instruction
               + INSTR_SIZE;                                              // because we will add another instruction (to avoid literal pool) at the end of the section

  codeInstruction((OC << 4) | M, A << 4 | B, (C << 4) | (getByte(offset, 1) & 0x0F), getByte(offset, 0));

  // Relocation
  int relTabOffset = myElf->sections[currSecId]->size + INSTR_SIZE + (myElf->sections[currSecId]->literalPool.size() - 1) * 4;
  newRelocation(relTabOffset, D, MyElf::ABSOLUTE, 0);
}


void Assembler::cleanup()
{
  for (Symbol *symbol: symbolTable) delete symbol;

  for (Section *section: sectionTable) delete section;

  fclose(asmFile);

  delete myElf;  
}

// ----------------------------------- DIRECTIVES ---------------------------------

void Assembler::_global(char *symbol)
{
  if (first)
  {
    setGlobal(symbol);
  }
}


void Assembler::_extern(char *symbol)
{
  if (first)
  {
    setGlobal(symbol);
  }
}


void Assembler::_section(char *symbol)
{
  if (first)
  {
    if (currSecId != 0)
    {
      sectionTable[currSecId]->size = locationCounter;
      myElf->sections[currSecId]->size = locationCounter;
    }

    locationCounter = 0;
    currSecId = sectionTable.size();
    currSecName = symbol;
    sectionTable.push_back(new Section(symbol));
    myElf->sections.push_back(new MyElf::Section(currSecId, symbol));
  }
  else
  {
    // If previous section used literal pool,
    // we need to insert pc <= pc + literealPool.size() * sizeof(int) instruction
    int litPoolSize = myElf->sections[currSecId]->literalPool.size() * sizeof(int);
    if (litPoolSize > 0)
    {
      codeInstruction((JMP_OC << 4) | JMP_M1, PC << 4, getByte(litPoolSize, 1) & 0x0F, getByte(litPoolSize, 0));
    }

    locationCounter = 0;
    for (int i = 0; i < sectionTable.size(); i++)

      if (sectionTable[i]->name == symbol)
      {
        currSecId = i;
        break;
      }
    currSecName = symbol;
  }
}


void Assembler::_word(char *symbol)
{
  if (first)
  {
    locationCounter += 4;
  }
  else
  {
    newRelocation(locationCounter, symbol, MyElf::ABSOLUTE, 0);

    for (int i = 0; i < 4; i++)
    {
      myElf->sections[currSecId]->memory.push_back(0);
    }

    locationCounter += 4;
  }
}


void Assembler::_word(int literal)
{
  if (first)
  {
    locationCounter += 4;
  }
  else
  {
    for (int i = 0; i < 4; i++)
    {
      myElf->sections[currSecId]->memory.push_back(getByte(literal, i));
    }

    locationCounter += 4;
  }
}


void Assembler::_skip(int literal)
{
  if (first)
  {
    locationCounter += literal;
  }
  else
  {
    for (int i = 0; i < literal; i++)
    {
      myElf->sections[currSecId]->memory.push_back(0);
    }
    locationCounter += literal;
  }
}


void Assembler::_end()
{
  if (first)
  {
    if (currSecId != 0)
    {
      sectionTable[currSecId]->size = locationCounter;
      myElf->sections[currSecId]->size = locationCounter;
    }

    first = false;
    locationCounter = 0;
    createMyElfSymbolTable();
  }
  else
  {
    // If previous section used literal pool,
    // we need to insert pc <= pc + literealPool.size() * sizeof(int) instruction
    int litPoolSize = myElf->sections[currSecId]->literalPool.size() * sizeof(int);
    if (litPoolSize > 0)
    {
      codeInstruction((JMP_OC << 4) | JMP_M1, PC << 4, getByte(litPoolSize, 1) & 0x0F, getByte(litPoolSize, 0));
    }

    myElf->print();
    // cout << "File assembled successfuly!" << endl;
    exit(0);
  }
}

// -------------------------------------- LABEL --------------------------------------

void Assembler::_label(char *symbol)
{
  if (first)
  {

    int i = 0;
    for (; i < symbolTable.size(); i++)
    {
      if (symbolTable[i]->name == symbol)
      {
        if (symbolTable[i]->sectionId != 0)
        {
          cout << "Symbol " << symbol << " already defined here: section "
               << sectionTable[symbolTable[i]->sectionId]->name << ", byte " << symbolTable[i]->value << endl;
          exit(-1);
        }

        symbolTable[i]->sectionId = currSecId;
        symbolTable[i]->value = locationCounter;
        break;
      }
    }

    if (i == symbolTable.size())
    {
      symbolTable.push_back(new Symbol(symbol, locationCounter, false, currSecId));
    }
  }
}

// ------------------------------ CONTROLE INSTRUCTIONS ------------------------------

void Assembler::_halt()
{
  if (first)
  {
    locationCounter += INSTR_SIZE;
  }
  else
  {
    // 0b0000 0000 0000 0000 0000 0000 0000 0000
    codeInstruction(HALT_OC << 4, 0, 0, 0);
  }
}


void Assembler::_int()
{
  if (first)
  {
    locationCounter += INSTR_SIZE;
  }
  else
  {
    // 0b0001 0000 0000 0000 0000 0000 0000 0000
    codeInstruction(INT_OC << 4, 0, 0, 0);
  }
}


void Assembler::_iret()
{
  if (first)
  {
    locationCounter += INSTR_SIZE * 3; // Because it is pseudo instruction (translates to three instructions)
  }
  else
  {
    // Pseudo instruction, transletes to: pop pc; pop status;
    // But, if we pop into pc, pop status will never happen (because pc will be somewhere else)
    // Instead we need to resolve it into three instructions:

    // 1. gprA = gprB + D (gprA = gprB = SP, D = 8)
    codeInstruction((LD_OC << 4) | LD_GPR_M2, (SP << 4) | SP, getByte(8, 1) & 0x0F, getByte(8, 0));

    // 2. csrA = mem [gprB + gprC + D] (csrA = status, gprB = SP, D = -4)
    codeInstruction((LD_OC << 4) | LD_CSR_M3, (STATUS << 4) | SP, getByte(-4, 1) & 0x0F, getByte(-4, 0));

    // 3. gprA = mem[gprB + gprC + D] (gprA = PC, gprB = SP, D = -8)
    codeInstruction((LD_OC << 4) | LD_GPR_M3, (PC << 4) | SP, getByte(-8, 1) & 0x0F, getByte(-8, 0));
  }
}


void Assembler::_ret()
{
  // Pseudo instruction, transletes to: pop pc;
  if (first)
  {
    locationCounter += INSTR_SIZE;
  }
  else
  {
    // pop pc; We can use defined _pop(int) function because pc is gpr register
    _pop(PC);
  }
}

// ------------------------------ CALL/JUMP INSTRUCTIONS ------------------------------

void Assembler::_call(int literal)
{
  if (first)
  {
    locationCounter += INSTR_SIZE;
  }
  else
  {
    literalPoolProcessing(CALL_OC, CALL_M1, CALL_M2, 0, PC, 0, 0, literal);
  }
}


void Assembler::_call(char *symbol)
{
  if (first)
  {
    locationCounter += INSTR_SIZE;
  }
  else
  {
    literalPoolProcessing(CALL_OC, CALL_M2, PC, 0, 0, symbol);
  }
}


void Assembler::_jmp(int literal)
{
  if (first)
  {
    locationCounter += INSTR_SIZE;
  }
  else
  {
    literalPoolProcessing(JMP_OC, JMP_M1, JMP_M5, 0, PC, 0, 0, literal);
  }
}


void Assembler::_jmp(char *symbol)
{
  if (first)
  {
    locationCounter += INSTR_SIZE;
  }
  else
  {
    literalPoolProcessing(JMP_OC, JMP_M5, PC, 0, 0, symbol);
  }
}


void Assembler::_beq(int gpr1, int gpr2, int literal)
{
  if (first)
  {
    locationCounter += INSTR_SIZE;
  }
  else
  {
    literalPoolProcessing(JMP_OC, JMP_M2, JMP_M6, 0, PC, gpr1, gpr2, literal);
  }
}


void Assembler::_beq(int gpr1, int gpr2, char *symbol)
{
  if (first)
  {
    locationCounter += INSTR_SIZE;
  }
  else
  {
    literalPoolProcessing(JMP_OC, JMP_M6, PC, gpr1, gpr2, symbol);
  }
}


void Assembler::_bne(int gpr1, int gpr2, int literal)
{
  if (first)
  {
    locationCounter += INSTR_SIZE;
  }
  else
  {
    literalPoolProcessing(JMP_OC, JMP_M3, JMP_M7, 0, PC, gpr1, gpr2, literal);
  }
}


void Assembler::_bne(int gpr1, int gpr2, char *symbol)
{
  if (first)
  {
    locationCounter += INSTR_SIZE;
  }
  else
  {
    literalPoolProcessing(JMP_OC, JMP_M7, PC, gpr1, gpr2, symbol);
  }
}


void Assembler::_bgt(int gpr1, int gpr2, int literal)
{
  if (first)
  {
    locationCounter += INSTR_SIZE;
  }
  else
  {
    literalPoolProcessing(JMP_OC, JMP_M4, JMP_M8, 0, PC, gpr1, gpr2, literal);
  }
}


void Assembler::_bgt(int gpr1, int gpr2, char *symbol)
{
  if (first)
  {
    locationCounter += INSTR_SIZE;
  }
  else
  {
    literalPoolProcessing(JMP_OC, JMP_M8, PC, gpr1, gpr2, symbol);
  }
}

// ------------------------------ PUSH/POP INSTRUCTIONS ------------------------------

void Assembler::_push(int gpr)
{
  if (first)
  {
    locationCounter += INSTR_SIZE;
  }
  else
  {
    codeInstruction((ST_OC << 4) | ST_M3, (SP << 4), gpr << 4 | (getByte(-4, 1) & 0x0F), getByte(-4, 0));
  }
}


void Assembler::_pop(int gpr)
{
  if (first)
  {
    locationCounter += INSTR_SIZE;
  }
  else
  {
    codeInstruction((LD_OC << 4) | LD_GPR_M4, (gpr << 4) | SP, getByte(4, 1) & 0x0F, getByte(4, 0));
  }
}

// --------------------------------- ALU INSTRUCTIONS ---------------------------------

void Assembler::_xchg(int gprS, int gprD)
{
  if (first)
  {
    locationCounter += INSTR_SIZE;
  }
  else
  {
    codeInstruction(XCHG_OC << 4, gprS, gprD << 4, 0);
  }
}


void Assembler::_add(int gprS, int gprD)
{
  if (first)
  {
    locationCounter += INSTR_SIZE;
  }
  else
  {
    codeInstruction((ARI_OP_OC << 4) | ADD, (gprD << 4) | gprD, gprS << 4, 0);
  }
}


void Assembler::_sub(int gprS, int gprD)
{
  if (first)
  {
    locationCounter += INSTR_SIZE;
  }
  else
  {
    codeInstruction((ARI_OP_OC << 4) | SUB, (gprD << 4) | gprD, gprS << 4, 0);
  }
}


void Assembler::_mul(int gprS, int gprD)
{
  if (first)
  {
    locationCounter += INSTR_SIZE;
  }
  else
  {
    codeInstruction((ARI_OP_OC << 4) | MUL, (gprD << 4) | gprD, gprS << 4, 0);
  }
}


void Assembler::_div(int gprS, int gprD)
{
  if (first)
  {
    locationCounter += INSTR_SIZE;
  }
  else
  {
    codeInstruction((ARI_OP_OC << 4) | DIV, (gprD << 4) | gprD, gprS << 4, 0);
  }
}


void Assembler::_not(int gpr)
{
  if (first)
  {
    locationCounter += INSTR_SIZE;
  }
  else
  {
    codeInstruction((LOG_OP_OC << 4) | NOT, (gpr << 4) | gpr, 0, 0);
  }
}


void Assembler::_and(int gprS, int gprD)
{
  if (first)
  {
    locationCounter += INSTR_SIZE;
  }
  else
  {
    codeInstruction((LOG_OP_OC << 4) | AND, (gprD << 4) | gprD, gprS << 4, 0);
  }
}


void Assembler::_or(int gprS, int gprD)
{
  if (first)
  {
    locationCounter += INSTR_SIZE;
  }
  else
  {
    codeInstruction((LOG_OP_OC << 4) | OR, (gprD << 4) | gprD, gprS << 4, 0);
  }
}


void Assembler::_xor(int gprS, int gprD)
{
  if (first)
  {
    locationCounter += INSTR_SIZE;
  }
  else
  {
    codeInstruction((LOG_OP_OC << 4) | XOR, (gprD << 4) | gprD, gprS << 4, 0);
  }
}


void Assembler::_shl(int gprS, int gprD)
{
  if (first)
  {
    locationCounter += INSTR_SIZE;
  }
  else
  {
    codeInstruction((SH_OP_OC << 4) | SHL, (gprD << 4) | gprD, gprS << 4, 0);
  }
}


void Assembler::_shr(int gprS, int gprD)
{
  if (first)
  {
    locationCounter += INSTR_SIZE;
  }
  else
  {
    codeInstruction((SH_OP_OC << 4) | SHR, (gprD << 4) | gprD, gprS << 4, 0);
  }
}

// -------------------------------------- LOAD --------------------------------------

void Assembler::_ldImm(int literal, int gprD)
{
  if (first)
  {
    locationCounter += INSTR_SIZE;
  }
  else
  {
    literalPoolProcessing(LD_OC, LD_GPR_M2, LD_GPR_M3, gprD, gprD, 0, PC, literal);
  }
}


void Assembler::_ldImm(char *symbol, int gprD)
{
  if (first)
  {
    locationCounter += INSTR_SIZE;
  }
  else
  {
    literalPoolProcessing(LD_OC, LD_GPR_M3, gprD, 0, PC, symbol);
  }
}


void Assembler::_ldMemDir(int literal, int gprD)
{
  if (first)
  {
    locationCounter += INSTR_SIZE * 4;
  }
  else
  {
    if (literal < D_MAX && literal > D_MIN)
    {
      codeInstruction((LD_OC) << 4 | LD_GPR_M3, (gprD << 4), getByte(literal, 1) & 0x0F, getByte(literal, 0));
    }
    else
    {
      // Push r13
      _push(13);

      // First instruction; load literal in r13
      literalPoolProcessing(LD_OC, 0, LD_GPR_M3, 0, 13, PC, 0, literal);

      // Second instruction; gprD <= mem[r13]
      _ldRegInd(13, gprD);

      // Pop r13
      _pop(13);
    }
  }
}


void Assembler::_ldMemDir(char *symbol, int gprD)
{
  if (first)
  {
    locationCounter += INSTR_SIZE * 4;
  }
  else
  {
    // Push r13
    _push(13);

    // First instruction; load literal in r13
    literalPoolProcessing(LD_OC, LD_GPR_M3, 13, PC, 0, symbol);

    // Second instruction; gprD <= mem[r13]
    _ldRegInd(13, gprD);

    // Pop r13
    _pop(13);
  }
}


void Assembler::_ldReg(int gprS, int gprD)
{
  if (first)
  {
    locationCounter += INSTR_SIZE;
  }
  else
  {
    codeInstruction((LD_OC << 4) | LD_GPR_M2, (gprD << 4) | gprS, 0, 0);
  }
}


void Assembler::_ldRegInd(int gprS, int gprD)
{
  if (first)
  {
    locationCounter += INSTR_SIZE;
  }
  else
  {
    codeInstruction((LD_OC << 4) | LD_GPR_M3, (gprD << 4) | gprS, 0, 0);
  }
}


void Assembler::_ldRegIndOff(int gprS, int literal, int gprD)
{
  if (first)
  {
    locationCounter += INSTR_SIZE;
  }
  else
  {
    if (literal < D_MAX)
    {
      codeInstruction((LD_OC << 4) | LD_GPR_M3, (gprD << 4) | gprS, getByte(literal, 1) & 0x0F, getByte(literal, 0));
    }
    else
    {
      cout << "Literal in instruction load with register indirect address mode can't fit into instruction!";
      exit(-3);
    }
  }
}


void Assembler::_ldRegIndOff(int gprS, char *symbol, int gprD)
{
  if (first)
  {
    locationCounter += INSTR_SIZE;
  }
  else
  {
    cout << "Literal in instruction load with register indirect address mode can't be symbol!" << endl;
    exit(-3);
  }
}

// -------------------------------------- STORE --------------------------------------

void Assembler::_stImm(int gpr, int literal)
{
  cout << "Invalid combination of instruction and address mode (store and immidiate)" << endl;
  exit(-3);
}


void Assembler::_stImm(int gpr, char *symbol)
{
  cout << "Invalid combination of instruction and address mode (store and immidiate)" << endl;
  exit(-3);
}


void Assembler::_stMemDir(int gprS, int literal)
{
  if (first)
  {
    locationCounter += INSTR_SIZE;
  }
  else
  {
    literalPoolProcessing(ST_OC, ST_M1, ST_M2, 0, PC, 0, gprS, literal);
  }
}


void Assembler::_stMemDir(int gprS, char *symbol)
{
  if (first)
  {
    locationCounter += INSTR_SIZE;
  }
  else
  {
    literalPoolProcessing(ST_OC, ST_M2, PC, 0, gprS, symbol);
  }
}


void Assembler::_stReg(int gprS, int gprD)
{
  if (first)
  {
    locationCounter += INSTR_SIZE;
  }
  else
  {
    codeInstruction((LD_OC << 4) | LD_GPR_M2, (gprS << 4) | gprD, 0, 0);
  }
}


void Assembler::_stRegInd(int gprS, int gprD)
{
  if (first)
  {
    locationCounter += INSTR_SIZE;
  }
  else
  {
    codeInstruction((ST_OC << 4) | ST_M1, gprD, gprS << 4, 0);
  }
}


void Assembler::_stRegIndOff(int gprS, int gprD, int literal)
{
  if (first)
  {
    locationCounter += INSTR_SIZE;
  }
  else
  {
    if (literal < D_MAX)
    {
      codeInstruction((ST_OC << 4) | ST_M1, gprD, (gprS << 4) | (getByte(literal, 1) & 0x0F), getByte(literal, 0));
    }
    else
    {
      cout << "Literal in instruction store with register indirect address mode can't fit into instruction!";
      exit(-3);
    }
  }
}


void Assembler::_stRegIndOff(int gprS, int gprD, char *symbol)
{
  if (first)
  {
    locationCounter += INSTR_SIZE;
  }
  else
  {
    cout << "Literal in instruction store with register indirect address mode can't be symbol!" << endl;
    exit(-3);
  }
}

// ------------------------------------ CSRRD/CSRWR ------------------------------------

void Assembler::_csrrd(int csrS, int gprD)
{
  if (first)
  {
    locationCounter += INSTR_SIZE;
  }
  else
  {
    codeInstruction((LD_OC << 4) | LD_GPR_M1, (gprD << 4) | csrS, 0, 0);
  }
}


void Assembler::_csrwr(int gprS, int csrD)
{
  if (first)
  {
    locationCounter += INSTR_SIZE;
  }
  else
  {
    codeInstruction((LD_OC << 4) | LD_CSR_M1, (csrD << 4) | gprS, 0, 0);
  }
}

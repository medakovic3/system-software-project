#include "../inc/emulator.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>

unordered_map<unsigned, char> Emulator::memory;

vector<unsigned> Emulator::gpr;
vector<unsigned> Emulator::csr;

char Emulator::OC;
char Emulator::M;
char Emulator::A;
char Emulator::B;
char Emulator::C;
char Emulator::D1;
char Emulator::D2_D3;
int Emulator::D;

string Emulator::message;

void Emulator::loadMemoryContent(string inputFileName) {

  ifstream inputFile(inputFileName);
  if (!inputFile) {
    cout << "Failed to open a file!" << endl;
    exit(-2);
  }

  string line;
  unsigned int address;
  char colon;
  int value;

  while(getline(inputFile, line)) {

    istringstream iss(line);

    iss >> hex >> address;
    iss >> colon;

    while (iss >> hex >> value)
      memory[address++] = value;
    
  }

  inputFile.close();
}


void Emulator::init() {

  for (int i = 0; i < NUM_OF_GPR - 1; i++) gpr.push_back(0); // All but pc
  gpr.push_back(PC_INIT);

  for (int i = 0; i < NUM_OF_CSR; i++) csr.push_back(0);
}


bool Emulator::fetchInstruction() {

  if (memory.find(gpr[pc]) == memory.end()) {
    message = "Emulated processor program counter was on invalid address: " + gpr[pc] + '\n';
    return false;
  }

  OC    = (memory[gpr[pc]] >> 4)  & LOWER_4_BITS;
  M     =  memory[gpr[pc]++]      & LOWER_4_BITS;
  A     = (memory[gpr[pc]] >> 4)  & LOWER_4_BITS;
  B     = (memory[gpr[pc]++])     & LOWER_4_BITS;
  C     = (memory[gpr[pc]] >> 4)  & LOWER_4_BITS;
  D1    = (memory[gpr[pc]++]      & LOWER_4_BITS) << 4;
  D2_D3 = (memory[gpr[pc]++]);

  D = ((int)D1 << 4) | ((int)D2_D3 & 0xFF);

  switch (OC) {
    case HALT:  return _halt();
    case INT:   _int();   break; 
    case CALL:  _call();  break;
    case JMP:   _jmp();   break;
    case XCHG:  _xchg();  break;
    case ARI:   _ari();   break;
    case LOG:   _log();   break;
    case SH:    _sh();    break;
    case ST:    _st();    break;
    case LD:    _ld();    break;
    default:    return wrongOC();
  }

  return true;
}

bool Emulator::wrongOC() {
  message = "Emulated processor encountered instruction with invalid operation code!\n";
  return false;
}

bool Emulator::_halt() {
  message = "Emulated processor executed halt instruction\n";
  return false; 
}

void Emulator::_int() {
  push(csr[status]);  
  push(gpr[pc]);
  csr[cause] = 4; 
  csr[status] &= ~0x1;
  gpr[pc] = csr[handler]; 
}

void Emulator::_call() {

  push(gpr[pc]);
  int newPC;
  switch (M) {
    case CALL_M1: newPC = gpr[A] + gpr[B] + D; break;
    case CALL_M2: newPC = fetchData(gpr[A] + gpr[B] + D); break;
  }
  gpr[pc] = newPC;
}

void Emulator::_jmp() {

  int newPC = gpr[pc];
  int imm = gpr[A] + D;
  int mem = fetchData(gpr[A] + D);
  int condition = (int)gpr[B] - (int)gpr[C];

  switch(M) {
    case JMP_M1 :                     newPC = imm; break;
    case JMP_M2 : if (!condition)     newPC = imm; break;
    case JMP_M3 : if (condition)      newPC = imm; break;
    case JMP_M4 : if (condition > 0)  newPC = imm; break;
    case JMP_M5 :                     newPC = mem; break;
    case JMP_M6 : if (!condition)     newPC = mem; break;
    case JMP_M7 : if (condition)      newPC = mem; break;
    case JMP_M8 : if (condition > 0)  newPC = mem; break;
  }

  gpr[pc] = newPC;
}

void Emulator::_xchg() {
  int temp = gpr[B];
  gpr[B] = gpr[C];
  gpr[C] = gpr[B];
}

void Emulator::_ari() {
  switch(M) {
    case ADD: gpr[A] = gpr[B] + gpr[C]; break;
    case SUB: gpr[A] = gpr[B] - gpr[C]; break;
    case MUL: gpr[A] = gpr[B] * gpr[C]; break;
    case DIV: gpr[A] = gpr[B] / gpr[C]; break;
  }
}

void Emulator::_log() {
  switch(M) {
    case NOT: gpr[A] = ~gpr[B];         break;
    case AND: gpr[A] = gpr[B] & gpr[C]; break;
    case OR: gpr[A] = gpr[B] | gpr[C];  break;
    case XOR: gpr[A] = gpr[B] ^ gpr[C]; break;
  }
}

void Emulator::_sh() {
  switch(M) {
    case SHL: gpr[A] = gpr[B] << gpr[C]; break;
    case SHR: gpr[A] = gpr[B] >> gpr[C]; break;
  }
}

void Emulator::_st() {
  switch(M) {
    case ST_M1: insertData(gpr[A] + gpr[B] + D, gpr[C]);            break;
    case ST_M2: insertData(fetchData(gpr[A] + gpr[B] + D), gpr[C]); break;
    case ST_M3: gpr[A] = gpr[A] + D; insertData(gpr[A], gpr[C]);    break;
  }
}


void Emulator::_ld()
{
  switch(M) {
    case LD_M1: gpr[A] = csr[B];                                  break;
    case LD_M2: gpr[A] = gpr[B] + D;                              break;
    case LD_M3: gpr[A] = fetchData(gpr[B] + gpr[C] + D);          break;
    case LD_M4: gpr[A] = fetchData(gpr[B]); gpr[B] = gpr[B] + D;  break;
    case LD_M5: csr[A] = gpr[B];                                  break;
    case LD_M6: csr[A] = csr[B] | D;                              break;
    case LD_M7: csr[A] = fetchData(gpr[B] + gpr[C] + D);          break;
    case LD_M8: csr[A] = fetchData(gpr[B]); gpr[B] = gpr[B] + D;  break;
  }
}

void Emulator::push(int value) {
  for (int i = 3; i >= 0; i--)
    memory[--gpr[sp]] = getByte(value, i);  
}

char Emulator::getByte(int value, int byteIndex) {
  return (value >> (byteIndex * 8)) & 0xFF;
}

int Emulator::fetchData(int address) {

  int data = 0;

  for (int i = 3; i >= 0; i--) {
    data <<= 8;
    data |= (unsigned char)memory[address + i];
  }

  return data;
}

void Emulator::insertData(int address, int value) {
  for (int i = 0; i < 4; i++) {
    memory[address + i] = getByte(value, i);
  }
}

void Emulator::printProcossorState() {
  cout << message;
  cout << "Emulated processor state:";

  for (int i = 0; i < gpr.size(); i++) {
    if (i % 4 == 0) cout << endl;
    cout << "r" << dec << i << "=0x" << hex << setw(8) << setfill('0') << gpr[i] << '\t';
  }
  cout << endl;
}



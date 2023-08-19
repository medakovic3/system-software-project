#if !defined(ASSEMBLER)
#define ASSEMBLER

#include "myElf.h"

class Assembler {

public:

  // directives
  static void _global(char *);
  static void _extern(char *);
  static void _section(char *);
  static void _word(char *);
  static void _word(int);
  static void _skip(int);
  static void _end();

  // label
  static void _label(char *);

  // instructions
  static void _halt();
  static void _int();
  static void _iret();
  static void _ret();
  
  static void _call(int);
  static void _call(char *);

  static void _jmp(int);
  static void _jmp(char *);

  static void _beq(int, int, int);
  static void _beq(int, int, char *);

  static void _bne(int, int, int);
  static void _bne(int, int, char *);

  static void _bgt(int, int, int);
  static void _bgt(int, int, char *);

  static void _push(int);
  static void _pop(int);

  static void _xchg(int, int);
  static void _add(int, int);
  static void _sub(int, int);
  static void _mul(int, int);
  static void _div(int, int);
  static void _not(int);
  static void _and(int, int);
  static void _or(int, int);
  static void _xor(int, int);
  static void _shl(int, int);
  static void _shr(int, int);

  static void _ldImm(int, int);
  static void _ldImm(char *, int);
  static void _ldMemDir(int, int);
  static void _ldMemDir(char *, int);
  static void _ldReg(int, int);
  static void _ldRegInd(int, int);
  static void _ldRegIndOff(int, int, int);
  static void _ldRegIndOff(int, char *, int);

  static void _stImm(int, int);                // invalid
  static void _stImm(int, char *);             // invalid
  static void _stMemDir(int, int);
  static void _stMemDir(int, char *);
  static void _stReg(int, int);                // invalid
  static void _stRegInd(int, int);
  static void _stRegIndOff(int, int, int);
  static void _stRegIndOff(int, int, char *);

  static void _csrrd(int, int);
  static void _csrwr(int, int);

  // Helper functions
  static void setGlobal(char *);
  static void init();
  static void createMyElfSymbolTable();
  static void newRelocation(int, char *, MyElf::RelocationTypes, int);
  static char getByte(int, int);
  static void codeInstruction(char, char, char, char);
  static void literalPoolProcessing(char, char, char, char, char, char, char, int);
  static void literalPoolProcessing(char, char, char, char, char, char *);

  // Deallocation
  static void cleanup();

  // Data structures

  static FILE *asmFile;
  static unsigned locationCounter;
  static bool first;
  static int currSecId;
  static string currSecName;
  static MyElf *myElf;

  struct Symbol {
    string name;
    int value;
    bool global;
    int sectionId;

    Symbol(string n, int v = 0, bool g = false, int sId = 0) {
      name = n;
      value = v;
      global = g;
      sectionId = sId;
    }
  };

  static vector<Symbol*> symbolTable;

  struct Section {
    string name;
    int size;

    Section(string n) {
      name = n;
      size = 0;
    }
  };

  static vector<Section*> sectionTable;

};



#endif // ASSEMBLER



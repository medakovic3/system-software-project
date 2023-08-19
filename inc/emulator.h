#if !defined(EMULATOR)
#define EMULATOR

#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

#define NUM_OF_GPR    16
#define NUM_OF_CSR    3

#define PC_INIT       0x40000000
#define pc            15
#define sp            14
#define status        0
#define handler       1
#define cause         2

#define LOWER_4_BITS  0xF

class Emulator {
public:

  static void loadMemoryContent(string inputFileName);

  static void init();

  static bool fetchInstruction();

  // instructions
  static bool _halt();
  static void _int();
  
  static void _call();

  static void _jmp();

  static void _xchg();

  static void _ari();

  static void _log();
  static void _not();
  static void _and();
  static void _or();
  static void _xor();

  static void _sh();

  static void _ld();

  static void _st();

  static bool wrongOC();


  static void printProcossorState();

private:

  static unordered_map<unsigned, char> memory;

  static vector<unsigned> gpr;
  static vector<unsigned> csr;

  static string message;

  static char OC;
  static char M;
  static char A;
  static char B;
  static char C;
  static char D1;
  static char D2_D3;
  static int D;

  static void push(int);

  static char getByte(int, int);
  static int fetchData(int);
  static void insertData(int, int);
};

// -------------------------------- OPERATION CODES ------------------------------

#define HALT      0x0
#define INT       0x1
#define CALL      0x2
#define JMP       0x3
#define XCHG      0x4
#define ARI       0x5
#define LOG       0x6
#define SH        0x7
#define ST        0x8
#define LD        0x9

// ----------------------------------- LOAD MODES ---------------------------------

#define LD_M1     0x0 //  gpr[A]<=csr[B];
#define LD_M2     0x1 //  gpr[A]<=gpr[B]+D;
#define LD_M3     0x2 //  gpr[A]<=mem32[gpr[B]+gpr[C]+D];
#define LD_M4     0x3 //  gpr[A]<=mem32[gpr[B]]; gpr[B]<=gpr[B]+D;
#define LD_M5     0x4 //  csr[A]<=gpr[B];
#define LD_M6     0x5 //  csr[A]<=csr[B]|D;
#define LD_M7     0x6 //  csr[A]<=mem32[gpr[B]+gpr[C]+D];
#define LD_M8     0x7 //  csr[A]<=mem32[gpr[B]]; gpr[B]<=gpr[B]+D;

// ---------------------------------- STORE MODES ---------------------------------

#define ST_M1     0x0 //  mem32[gpr[A]+gpr[B]+D]<=gpr[C];
#define ST_M2     0x2 //  mem32[mem32[gpr[A]+gpr[B]+D]]<=gpr[C];
#define ST_M3     0x1 //  gpr[A]<=gpr[A]+D; mem32[gpr[A]]<=gpr[C];

// ----------------------------------- CALL MODES ---------------------------------

#define CALL_M1   0x0 //  push pc; pc<=gpr[A]+gpr[B]+D;
#define CALL_M2   0x1 //  push pc; pc<=mem32[gpr[A]+gpr[B]+D];

// ----------------------------------- JUMP MODES ---------------------------------

#define JMP_M1    0x0 //  pc<=gpr[A]+D;
#define JMP_M2    0x1 //  if (gpr[B] == gpr[C]) pc<=gpr[A]+D;
#define JMP_M3    0x2 //  if (gpr[B] != gpr[C]) pc<=gpr[A]+D;
#define JMP_M4    0x3 //  if (gpr[B] signed > gpr[C]) pc<=gpr[A]+D;
#define JMP_M5    0x8 //  pc<=mem[gpr[A]+D];
#define JMP_M6    0x9 //  if (gpr[B] == gpr[C]) pc<=mem[gpr[A]+D];
#define JMP_M7    0xA //  if (gpr[B] != gpr[C]) pc<=mem[gpr[A]+D];
#define JMP_M8    0xB //  if (gpr[B] signed > gpr[C]) pc<=mem[gpr[A]+D];

// ------------------------------------ ALU MODES ---------------------------------

#define ADD       0x0 //  gpr[A]<=gpr[B] + gpr[C];
#define SUB       0x1 //  gpr[A]<=gpr[B] - gpr[C];
#define MUL       0x2 //  gpr[A]<=gpr[B] * gpr[C];
#define DIV       0x3 //  gpr[A]<=gpr[B] / gpr[C];
#define NOT       0x0 //  gpr[A]<=~gpr[B];
#define AND       0x1 //  gpr[A]<=gpr[B] & gpr[C];
#define OR        0x2 //  gpr[A]<=gpr[B] | gpr[C];
#define XOR       0x3 //  gpr[A]<=gpr[B] ^ gpr[C];
#define SHL       0x0 //  gpr[A]<=gpr[B] << gpr[C];  
#define SHR       0x1 //  gpr[A]<=gpr[B] >> gpr[C];  

#endif //EMULATOR
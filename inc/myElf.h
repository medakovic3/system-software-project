#if !defined(MYELF)
#define MYELF

#include <string>
#include <vector>
#include <fstream>
using namespace std;

class MyElf
{
public:

  ~MyElf()
  {
    for (Symbol *symbol : symbolTable)
      delete symbol;

    for (RelocationTable *rt : relocationTables)
    {
      for (Relocation *rel : rt->relocations)
      {
        delete rel;
      }
      delete rt;
    }

    for (Section *section : sections)
      delete section;

    outputFile.close();
  }

  // -------------- Symbol table --------------

  struct Symbol
  {
    string name;
    int value;
    bool isGlobal;
    bool isSection;
    int sectionId;

    Symbol(string n, int v, bool isG, bool isS, int sId)
    {
      name = n;
      value = v;
      isGlobal = isG;
      isSection = isS;
      sectionId = sId;
    }
  };

  vector<Symbol *> symbolTable;

  // ------------ Relocation tables ------------

  enum RelocationTypes
  {
    RELATIVE,
    ABSOLUTE
  };

  struct Relocation
  {
    int offset;
    RelocationTypes type;
    int symbolId;
    int addend;

    Relocation(int off, RelocationTypes t, int sId, int add)
    {
      offset = off;
      type = t;
      symbolId = sId;
      addend = add;
    }
  };

  struct RelocationTable
  {
    int sectionId;
    string sectionName;
    vector<Relocation *> relocations;

    RelocationTable(int sId, string sName)
    {
      sectionId = sId;
      sectionName = sName;
    }
  };

  vector<RelocationTable *> relocationTables;

  // ------------ Sections content ------------

  struct Section
  {
    int secId;
    string sectionName;
    vector<char> memory;
    int size;
    vector<int> literalPool;
    bool loaded;  // used by linker only
    MyElf *myElf; // used by linker only

    Section(int sId, string sName)
    {
      secId = sId;
      sectionName = sName;
    }
  };

  vector<Section *> sections;

  ofstream outputFile;

  int symbolId(char *);

  int secRelTabId(int);

  Section *findSection(int);
  
  void print();

  static MyElf *read(FILE *);

  static void loadSymbolTable(FILE *, MyElf *);

  static void loadRelocationTables(FILE *, MyElf *);

  static void loadSectionsContent(FILE *, MyElf *);
};

#endif
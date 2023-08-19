#if !defined(LINKER)
#define LINKER

#include "myElf.h"

class Linker
{
public:
  // Load elf files

  static bool loadElfFiles(vector<string>);

  // Sections
  static bool aragneSections();

  // Symbols
  static void calculateSymbolValues();

  // Relocations
  static void processRelocations();

  // Print output
  static void print(string);

  // Clean
  static void cleanup();

  // Data structures

  struct PlaceSection
  {
    string name;
    unsigned address;
  };

  static vector<PlaceSection *> placeSections;

  static vector<MyElf *> elfFiles;

  struct Section
  {
    MyElf::Section *section;
    unsigned startAddr;
    unsigned endAddr;
    Section *next;
    Section *prev;

    Section(MyElf::Section *s, unsigned sa)
    {
      section = s;
      startAddr = sa;
    }
  };

  static Section *firstSection;
  static Section *lastSection;

  // Sextions helpers

  static MyElf::Section *findSection(string name);
  static bool putSection(Section *section);
  static void updateSectionValue(Section *sec);
};

#endif // LINKER
#include "../inc/myElf.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <string>
#include <cstring>

// Print ElF file (assembler's output, linker's input)
void MyElf::print()
{
  // -------------------------------- Symbol table --------------------------------

  outputFile << "SYMBOL TABLE" << endl
             << endl;

  // Set the column widths
  const int colWidth = 15;

  // Print column headers
  outputFile << setw(2) << "Id"
             << setw(colWidth) << "Name"
             << setw(colWidth) << "Value"
             << setw(colWidth) << "Binding"
             << setw(colWidth) << "Type"
             << setw(colWidth) << "SectionId" << endl;

  // Print horizontal line
  outputFile << setfill('-') << setw(6 * colWidth) << "-" << setfill(' ') << endl;

  // Iterate over the symbolTable and print each Symbol
  for (int i = 0; i < symbolTable.size(); i++)
  {
    Symbol *symbol = symbolTable[i];
    if (!symbol->isSection && !symbol->isGlobal)
      continue; // Local symbols should not be in ELF symbol table
    outputFile << setw(2) << i
               << setw(colWidth) << symbol->name
               << setw(colWidth) << symbol->value
               << setw(colWidth) << (symbol->isGlobal ? "global" : "local")
               << setw(colWidth) << (symbol->isSection ? "section" : "symbol")
               << setw(colWidth) << symbol->sectionId << endl;
  }

  outputFile.flush();

  outputFile << setfill('_') << setw(6 * colWidth) << "_" << setfill(' ') << endl;

  // ------------------------------ Relocation tables ------------------------------

  outputFile << "RELOCATION TABLES" << endl
             << endl;

  for (const RelocationTable *relocationTable : relocationTables)
  {
    outputFile << "Section: " << relocationTable->sectionName << endl;

    outputFile << setw(4) << "Offset"
               << setw(colWidth) << "Type"
               << setw(colWidth) << "SymbolId"
               << setw(colWidth) << "Addend" << endl;

    outputFile << setfill('-') << setw(4 * colWidth) << "-" << setfill(' ') << endl;

    for (int i = 0; i < relocationTable->relocations.size(); i++)
    {
      Relocation *relocation = relocationTable->relocations[i];
      outputFile << setw(5) << relocation->offset
                 << setw(colWidth + 1) << (relocation->type == RELATIVE ? "RELATIVE" : "ABSOLUTE")
                 << setw(colWidth) << relocation->symbolId
                 << setw(colWidth) << relocation->addend << endl;
    }

    outputFile.flush();

    outputFile << endl;
  }

  outputFile << setfill('_') << setw(6 * colWidth) << "_" << setfill(' ') << endl;

  // ------------------------------ Sections content ------------------------------

  outputFile << "SECTIONS CONTENT" << endl
             << endl;

  const int bytesPerRow = 8;

  for (const Section *section : sections)
  {
    if (section->secId == 0)
      continue;
    outputFile << "Section: " << section->sectionName << endl;

    // Print memory vector
    int memorySize = section->memory.size();
    for (int i = 0; i < memorySize; i += bytesPerRow)
    {
      int j = 0;
      for (; j < bytesPerRow && i + j < memorySize; j++)
      {
        unsigned char byte = static_cast<unsigned char>(section->memory[i + j]);
        outputFile << setw(2) << setfill('0') << hex << static_cast<int>(byte) << " ";
      }
      if (j == 8)
      {
        outputFile << endl;
      }
    }

    if ((memorySize / 4) % 2 == 1 && section->literalPool.size() == 0)
      outputFile << endl;

    // Print literalPool vector
    int literalPoolSize = section->literalPool.size();
    for (int i = 0; i < literalPoolSize; i++)
    {
      int literal = section->literalPool[i];
      for (int j = 0; j < 4; j++)
      {
        unsigned char byte = static_cast<unsigned char>(literal >> (j * 8));
        outputFile << setw(2) << setfill('0') << hex << static_cast<int>(byte) << " ";
      }
      if (i % 2 == 1 - ((memorySize / 4) % 2))
        outputFile << endl;
    }
    if (((memorySize / 4) + literalPoolSize) % 2 == 1 && section->literalPool.size() != 0)
      outputFile << endl;

    outputFile << endl;
  }
}


// Retrun symbol id by name (if name doesn't exist in symbol table return -1)
int MyElf::symbolId(char *symbol)
{
  for (int i = 0; i < symbolTable.size(); i++)
  {
    if (symbol == symbolTable[i]->name)
      return i;
  }

  return -1;
}


// Return section place in relocation tables by section id (if id doesn't exist return -1)
int MyElf::secRelTabId(int secId)
{
  for (int i = 0; i < relocationTables.size(); i++)
  {
    if (relocationTables[i]->sectionId == secId)
      return i;
  }
  return -1;
}


// Return pointer to the section found by id
MyElf::Section *MyElf::findSection(int id)
{
  for (Section *s : sections)
  {
    if (s->secId == id)
      return s;
  }
  return nullptr;
}


// Read from ELF file (assembler's output, linker's input)
MyElf *MyElf::read(FILE *inputFile)
{
  if (inputFile == nullptr)
  {
    return nullptr; // Invalid file
  }

  MyElf *myElf = new MyElf();
  loadSymbolTable(inputFile, myElf);
  loadRelocationTables(inputFile, myElf);
  loadSectionsContent(inputFile, myElf);

  return myElf;
}


void MyElf::loadSymbolTable(FILE *inputFile, MyElf *myElf)
{
  char line[256];

  // Skip the "SYMBOL TABLE" line
  fgets(line, sizeof(line), inputFile);

  // Skip empty line
  fgets(line, sizeof(line), inputFile);

  // Read the table header
  fgets(line, sizeof(line), inputFile);

  // Skip the header line
  fgets(line, sizeof(line), inputFile);

  // Read each symbol and add it to the symbol table
  while (fgets(line, sizeof(line), inputFile))
  {
    if (line[0] == '_')
      break;
    
    int id, value, sectionId;
    char name[256], binding[256], type[256];
    sscanf(line, "%d %s %d %s %s %d", &id, name, &value, binding, type, &sectionId);

    bool isGlobal = (strcmp(binding, "global") == 0);
    bool isSection = (strcmp(type, "section") == 0);

    myElf->symbolTable.push_back(new Symbol(name, value, isGlobal, isSection, sectionId));
  }
}


void MyElf::loadRelocationTables(FILE *inputFile, MyElf *myElf)
{
  char line[256];

  // Skip the "RELOCATION TABLES" line
  fgets(line, sizeof(line), inputFile);

  // Skip empty line
  fgets(line, sizeof(line), inputFile);

  // Read each relocation table
  while (fgets(line, sizeof(line), inputFile))
  {
    if (line[0] == '_')
      break;

    // Read the section name
    char sectionName[256];
    sscanf(line, "Section: %s", sectionName);

    // Find the corresponding section in the symbol table
    int sectionId = myElf->symbolId(sectionName);

    if (sectionId == -1)
    {
      // Section not found in the symbol table
      continue;
    }

    // Create a new relocation table
    RelocationTable *relocationTable = new RelocationTable(sectionId, sectionName);

    // Read the table header
    fgets(line, sizeof(line), inputFile);

    // Skip the header line
    fgets(line, sizeof(line), inputFile);

    // Read each relocation and add it to the relocation table
    while (fgets(line, sizeof(line), inputFile))
    {
      if (line[0] == '\n')
        break;
      int offset, symbolId, addend;
      char type[256];
      sscanf(line, "%d %s %d %d", &offset, type, &symbolId, &addend);

      RelocationTypes relocationType = (strcmp(type, "ABSOLUTE") == 0) ? ABSOLUTE : RELATIVE;

      relocationTable->relocations.push_back(new Relocation(offset, relocationType, symbolId, addend));
    }

    myElf->relocationTables.push_back(relocationTable);
  }
}


void MyElf::loadSectionsContent(FILE *inputFile, MyElf *myElf)
{
  // Skip the "SECTIONS CONTENT" line
  char line[256];
  fgets(line, sizeof(line), inputFile);

  // Skip empty line
  fgets(line, sizeof(line), inputFile);

  // Read each section content
  while (fgets(line, sizeof(line), inputFile))
  {
    // Read the section name
    char sectionName[256];
    sscanf(line, "Section: %s", sectionName);

    // Find the corresponding section in the symbol table
    int sectionId = myElf->symbolId(sectionName);

    if (sectionId == -1)
    {
      // Section not found in the symbol table
      continue;
    }

    // Create a new section
    Section *section = new Section(sectionId, sectionName);
    section->myElf = myElf;

    // Read the memory content
    while (fgets(line, sizeof(line), inputFile))
    {
      if (line[0] == '\n')
        break;

      string _hex;
      stringstream ss(line);
      while (ss >> _hex)
      {
        char byte = (char)stoi(_hex, nullptr, 16);
        section->memory.push_back(byte);
      }
    }

    section->size = section->memory.size();
    myElf->sections.push_back(section);
  }
}

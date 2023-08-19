#include "../inc/linker.h"
#include <iostream>
#include <algorithm>
#include <iomanip>

// ---------------------------- STATIC VARIABLES ----------------------------

vector<Linker::PlaceSection *> Linker::placeSections;
vector<MyElf *> Linker::elfFiles;
Linker::Section *Linker::firstSection;
Linker::Section *Linker::lastSection;

// ----------------------------- LOAD ELF FILES -----------------------------

bool Linker::loadElfFiles(vector<string> inputFiles)
{
  for (string inputFileName : inputFiles)
  {
    FILE *inputFile = fopen(inputFileName.c_str(), "r");
    if (!inputFile) return false;

    elfFiles.push_back(MyElf::read(inputFile));
    
    fclose(inputFile);
  }

  return elfFiles.size();
}

void Linker::print(string outputFileName)
{
  ofstream outputFile;
  outputFile.open(outputFileName, ofstream::out);

  int bytesPrinted = 0;

  for (Section *s = firstSection; s; s = s->next)
  {
    MyElf::Section *section = s->section;
    unsigned address = s->startAddr; // Starting address of a section
    unsigned prevAddr;               // Prevoiusly printed address

    // Check if last section output was aligned with 8
    if (bytesPrinted % 8 != 0)
      // Check if beginning of a new section is rigth behined end of a previous one
      if (address != prevAddr + 4)
        // If it is not, fill end of the line with zeros
        while (bytesPrinted % 8 != 0)
        {
          outputFile << " 00";
          bytesPrinted++;
        }
      else
        // If it is, set beginning of a new section 4 locations further
        address += 4;

    // Start of a new section
    for (int i = 0; i < section->memory.size(); i++)
    {
      if (bytesPrinted % 8 == 0)
      {

        if (!(s == firstSection && i == 0))
          outputFile << endl;

        outputFile << setw(8) << setfill('0') << hex << address << ':';
        prevAddr = address;
        address += 8;
      }

      outputFile << " " << setw(2) << setfill('0') << hex << (unsigned)((unsigned char)section->memory[i]);
      bytesPrinted++;
    }
  }

  while (bytesPrinted++ % 8 != 0)
    outputFile << " 00";

  outputFile.close();
}

void Linker::cleanup()
{
  for (PlaceSection *ps : placeSections)
    delete ps;

  for (MyElf *myElf : elfFiles)
    delete myElf;

  Section *s = firstSection->next, *p = firstSection;
  while (p)
  {
    delete p;
    p = s;
    if (s)
      s = s->next;
  }
}

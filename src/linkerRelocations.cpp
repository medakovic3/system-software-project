#include "../inc/linker.h"

char getByte(int value, int byteIndex)
{
  return (value >> (byteIndex * 8)) & 0xFF;
}

void Linker::processRelocations()
{

  for (MyElf *myElf : elfFiles)
  {
    for (MyElf::RelocationTable *rt : myElf->relocationTables)
    {
      for (MyElf::Relocation *rel : rt->relocations)
      {
        MyElf::Section *sec = myElf->findSection(rt->sectionId);
        int symValue = myElf->symbolTable[rel->symbolId]->value;
        int offset = rel->offset;
        MyElf::RelocationTypes type = rel->type;
        int secValue = myElf->symbolTable[rt->sectionId]->value;
        int addend = rel->addend;

        for (int i = 0; i < 4; i++)
        {
          int byteAbs = getByte(symValue + addend, i);
          int byteRel = getByte(symValue - (secValue + offset) + addend, i);
          sec->memory[offset + i] = type == MyElf::ABSOLUTE ? byteAbs : byteRel;
        }
      }
    }
  }
}
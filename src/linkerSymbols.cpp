#include "../inc/linker.h"
#include <iostream>

void Linker::calculateSymbolValues()
{
  // Update values of symbols defined in same file
  for (MyElf *myElf : elfFiles)
  {
    for (MyElf::Symbol *symbol : myElf->symbolTable)
    {
      if (!symbol->isSection && symbol->sectionId != 0)
      {
        symbol->value += myElf->symbolTable[symbol->sectionId]->value;
      }
    }
  }

  // Update values of extern symbols
  for (MyElf *myElf1 : elfFiles)
  {
    for (MyElf::Symbol *symbol1 : myElf1->symbolTable)
    {
      if (!symbol1->isSection && symbol1->sectionId == 0)
      {
        bool found = false;
        for (MyElf *myElf2 : elfFiles)
        {
          if (myElf1 != myElf2)
          {
            for (MyElf::Symbol *symbol2 : myElf2->symbolTable)
            {
              if (!symbol2->isSection && symbol2->sectionId != 0 && symbol2->isGlobal && symbol1->name == symbol2->name)
              {
                if (!found)
                {
                  found = true;
                  symbol1->value = symbol2->value;
                }
                else
                {
                  cout << "Symbol " << symbol1->name << " defined multiple times!" << endl;
                  exit(-1);
                }
              }
            }
          }
        }

        if (!found)
        {
          cout << "Symbol " << symbol1->name << " is not defined!" << endl;
          exit(-1);
        }
      }
    }
  }
}
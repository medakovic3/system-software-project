#include "../inc/linker.h"
#include <iostream>
#include <algorithm>

bool Linker::aragneSections()
{
  // Sort sections with place option
  sort(placeSections.begin(), placeSections.end(), [&](PlaceSection *s1, PlaceSection *s2)
       { return s1->address < s2->address; });

  // Sections with place option
  for (PlaceSection *ps : placeSections)
  {
    MyElf::Section *section = findSection(ps->name);

    if (!section)
      return false;

    section->loaded = true;
    Section *s = new Section(section, ps->address);

    if (!putSection(s))
      return false;

    // Update section value in symbol table
    updateSectionValue(s);
  }

  // Other sections
  for (MyElf *myElf : elfFiles)
  {
    for (MyElf::Section *section : myElf->sections)
    {
      if (!section->loaded)
      {
        section->loaded = true;
        Section *s = new Section(section, 0);
        putSection(s);
        updateSectionValue(s);
      }
    }
  }

  return true;
}


MyElf::Section *Linker::findSection(string name)
{
  for (MyElf *myElf : elfFiles)
  {
    for (MyElf::Section *section : myElf->sections)
    {
      if (name == section->sectionName)
      {
        return section;
      }
    }
  }
  return nullptr;
}


bool Linker::putSection(Section *section)
{
  if (!firstSection)
  {
    firstSection = lastSection = section;
    section->next = section->prev = nullptr;
  }
  else
  {
    // Check for overlapping (for sections with place option)
    if (lastSection->endAddr > section->startAddr && section->startAddr != 0)
      return false;

    // Chech if section with same name was already loaded
    for (Section *beforeSection = firstSection; beforeSection->next; beforeSection = beforeSection->next)
    {
      if (beforeSection->section->sectionName == section->section->sectionName)
      {

        section->next = beforeSection->next;
        section->prev = beforeSection->next->prev;
        beforeSection->next = section;
        section->next->prev = section;

        // Set start address
        section->startAddr = section->prev->endAddr;

        // Set end address
        section->endAddr = section->startAddr + section->section->size;

        // Update following section addresses
        for (Section *s = section->next; s; s = s->next)
        {
          s->startAddr += section->section->size;
          s->endAddr += section->section->size;

          // Update in symbol tables
          updateSectionValue(s);
        }

        return true;
      }
    }

    lastSection->next = section;
    section->prev = lastSection;
    lastSection = section;
    section->next = nullptr;

    // Set start address (for non-place sections)
    if (section->startAddr == 0)
      section->startAddr = section->prev->endAddr;
  }

  // Set end addres
  section->endAddr = section->startAddr + section->section->size;

  return true;
}


void Linker::updateSectionValue(Section *sec)
{
  int sectionId = sec->section->secId;
  sec->section->myElf->symbolTable[sectionId]->value = sec->startAddr;
}
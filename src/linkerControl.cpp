#include <iostream>
#include "../inc/linker.h"

vector<string> inputFiles;
string outputFile = "outputFile.hex";

void loadArguments(int argc, char **argv);

int main(int argc, char **argv)
{
  // ------------------------------------- Load arguments -------------------------------------

  loadArguments(argc, argv);

  // ----------------------------- Load files into MyElf objects -----------------------------

  if (!Linker::loadElfFiles(inputFiles))
  {
    cout << "Invalid input files names!" << endl;
    return -1;
  }

  // --------------------------------- Arrange sections order ---------------------------------

  if (!Linker::aragneSections())
  {
    cout << "Sections cannot be placed like this!" << endl;
    return -2;
  }

  // -------------------------------- Calculate symbol values --------------------------------

  Linker::calculateSymbolValues();

  // ---------------------------------- Process relovcations ----------------------------------

  Linker::processRelocations();

  // --------------------------------- Print into outputFile ---------------------------------

  Linker::print(outputFile);

  // ----------------------------------------- Clean -----------------------------------------

  Linker::cleanup();

  // cout << "Files linked successfuly!" << endl;

  return 0;
}


void loadArguments(int argc, char **argv)
{
  if (argc < 3)
  {
    cout << "Invalid number of arguments!\n";
    exit(-3);
  }

  bool hexOptionFound = false;

  for (int i = 1; i < argc; i++)
  {
    string arg = argv[i];

    if (arg == "-hex")
    {
      hexOptionFound = true;
    }
    else if (arg.find("-place=") == 0)
    {
      string placeArg = arg.substr(7); // Extract the substring after "-place="
      size_t atPos = placeArg.find('@');

      if (atPos != string::npos)
      {
        string name = placeArg.substr(0, atPos);
        string addressStr = placeArg.substr(atPos + 1);
        unsigned int address = stoul(addressStr, nullptr, 16);

        if (address % 8 != 0) {
          cout << "Section with place option " << name << " must start with address divisible by 8!" << endl;
          exit(-4);
        }

        Linker::PlaceSection *section = new Linker::PlaceSection{name, address};
        Linker::placeSections.push_back(section);
      }
    }
    else if (arg == "-o" && i + 1 < argc)
    {
      outputFile = argv[++i];
    }
    else
    {
      inputFiles.push_back(arg);
    }
  }

  if (!hexOptionFound)
  {
    cout << "Error: -hex argument is required.\n";
    exit(-1);
  }
}
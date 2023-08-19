#include <iostream>
#include <string>
#include <cstring>
#include <fstream>
#include "../inc/assembler.h"

using namespace std;

extern void yyparse();
extern FILE *yyin;

int main(int argc, char **argv)
{
  if (argc < 2)
  {
    cout << "Invalid number of arguments!\n";
    return -1;
  }

  int asmFileIndex = 1;
  char *outputFileName = strdup("file.o");

  if (argc == 4 && !strcmp(argv[1], "-o"))
  {
    asmFileIndex = 3;
    outputFileName = strdup(argv[2]);
  }

  // Open a file handle to a particular file:
  Assembler::asmFile = fopen(argv[asmFileIndex], "r");

  // Make sure it is valid:
  if (!Assembler::asmFile)
  {
    cout << "I can't open file " << argv[asmFileIndex] << endl;
    return -1;
  }

  // Set Flex to read from it instead of defaulting to STDIN:
  yyin = Assembler::asmFile;

  // Open a file handle to an output text file:
  Assembler::myElf->outputFile.open(outputFileName, ofstream::out);

  // Initialize assembler data structures
  Assembler::init();

  // First parse through the input:
  yyparse();

  // Return cursor to point at the begining of the file and set Flex to read from it
  fseek(Assembler::asmFile, 0, SEEK_SET);
  yyin = Assembler::asmFile;

  // Second parse through the input
  yyparse();

  // Deallocate allocated resources
  Assembler::cleanup();
}
#include <iostream>
#include <fstream>
#include "../inc/emulator.h"

using namespace std;

int main(int argc, char **argv) {

  if (argc < 2) {
    cout << "Input file isn't specified!" << endl;
    return -1;
  }

  string inputFileName = argv[1];

  Emulator::loadMemoryContent(inputFileName);

  Emulator::init();

  while (Emulator::fetchInstruction());

  Emulator::printProcossorState();

  return 0;
}
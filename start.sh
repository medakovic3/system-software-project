ASSEMBLER=./assembler
LINKER=./linker
EMULATOR=./emulator

${ASSEMBLER} -o main.o tests/main.s
${ASSEMBLER} -o math.o tests/math.s
${ASSEMBLER} -o handler.o tests/handler.s
${ASSEMBLER} -o isr_timer.o tests/isr_timer.s
${ASSEMBLER} -o isr_terminal.o tests/isr_terminal.s
${ASSEMBLER} -o isr_software.o tests/isr_software.s
${LINKER} -hex \
  -place=my_code@0x40000000 -place=math@0xF0000000 \
  -o program.hex \
  handler.o math.o main.o isr_terminal.o isr_timer.o isr_software.o
${EMULATOR} program.hex

#-------------------------------------------------------------------------------------

#bison -d misc/parser.y
#flex misc/lexer.l
#g++ parser.tab.c lex.yy.c src/myElf.cpp src/assembler.cpp src/assemblerControl.cpp -lfl -o assembler
#g++ src/myElf.cpp src/linkerRelocations.cpp src/linkerSections.cpp src/linkerSymbols.cpp src/linker.cpp src/linkerControl.cpp -o linker
#g++ src/emulator.cpp src/emulatorControl.cpp -o emulator


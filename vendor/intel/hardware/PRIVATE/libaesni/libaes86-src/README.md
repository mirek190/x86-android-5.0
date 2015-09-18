********************************
BUILDING libaes86.a
********************************
This build cannot be done as part of android build needs to be built
 seperately.

The libaes86.a was built using the source files in this folder
      \intel_aes.c
      \iaesx86.s
The archive static was built using yasm
************************
YASM BUILD INSTRUCTIONS
************************
    1. yasm -D__linux__ -g dwarf2 -f elf32 iaesx86.s -o iaesx86.o
    2. gcc -O3 -fPIC  -g -m32 -Iinclude/ -c intel_aes.c -o intel_aes.o
    3. ar -r libaes86.a *.o

NOTE :The include for the "gcc" command has two header files
  iaes_asm_interface.h and iaes.h used in libaesni.so


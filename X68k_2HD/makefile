# file.X makefile for X680x0 GCC (elf2x68k)

#É}ÉNÉç
CC = m68k-xelf-gcc

#CFLAGS =  -O -Wall -g
CFLAGS = -m68000 -O2 -finput-charset=utf-8 -fexec-charset=cp932

#LINKOPTS = -liocs -ldos
# -lsignal
LINKOPTS = -Ttext=0x6800 -s
# -specs=x68knodos.specs

.SUFFIXES: .o .c
.c.o:
	$(CC) $(CFLAGS) -c $<

#ê∂ê¨éËèá

all:file.X

OBJFILES = file.o

file.X : $(OBJFILES)
	$(CC) $(LINKOPTS) -o file.X $(OBJFILES)

file.o : file.c

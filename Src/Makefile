BINARY = out
OBJECTS = startup.o main.o pg.o pad.o string.o myproc.o mylib.o

all: $(BINARY)

$(BINARY): $(OBJECTS)
	ee-ld -O0 $(OBJECTS) -M -Ttext 8900000 -q -o $@ > rin.map
	outpatch
	elf2pbp outp "KUMAAAAH"

%.o : %.c
	ee-gcc -march=r4000 -O3 -fomit-frame-pointer -g -mgp32 -mlong32 -c $< -o $@

%.o : %.s
	ee-gcc -march=r4000 -g -mgp32 -c -xassembler -O -o $@ $<

clean:
	del /s /f *.o *.map

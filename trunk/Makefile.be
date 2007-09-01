#Makefile MUPEN64 for BeOS

#CC  		=gcc -O3 -mpentium -Wall -DEMU64_DEBUG
CC		=gcc -O3 -fomit-frame-pointer -funroll-loops -ffast-math -mpentium -Wall $(SDL_PATH) -pipe
#CC		=gcc -O3 -mpentium -Wall -g -pg
#CC 		=gcc -Wall -pipe -g -DEMU64_DEBUG $(SDL_PATH)

GLIDE_PATH	=-I/usr/include/glide
GL_PATH		=-I/usr/X11R6/include
SDL_PATH		=-I/boot/develop/tools/gnupro/include/

OBJ		=main/rom.o \
		r4300/r4300.o \
		r4300/cop0.o \
		r4300/special.o \
		r4300/regimm.o \
		r4300/exception.o \
		memory/tlb.o \
		memory/memory.o \
		memory/dma.o \
		r4300/interupt.o \
		rsp/rsp.o \
		rsp/special.o \
		r4300/cop1.o \
		r4300/tlb.o \
		graphic/common/common.o \
		r4300/cop1_w.o \
		r4300/cop1_s.o \
		r4300/cop1_d.o \
		r4300/recomp.o \
		memory/pif.o \
		r4300/bc.o \
		rsp/cop0.o \
		rsp/lwc2.o \
		rsp/regimm.o \
		rdp/rdp.o \
		rsp/swc2.o \
		rsp/cop2.o \
		rsp/vector.o
		
OBJ_SDL		=graphic/sdl/g_sdl.o \
		sound/sdl/sound.o

OBJ_X86         =r4300/x86/gr4300.o \
		r4300/x86/gcop0.o \
		r4300/x86/assemble.o \
		r4300/x86/gcop1.o \
		r4300/x86/gcop1_s.o \
		r4300/x86/gcop1_d.o \
		r4300/x86/gtlb.o \
		r4300/x86/gregimm.o \
		r4300/x86/gspecial.o \
		r4300/x86/gcop1_w.o \
		r4300/x86/debug.o \
		r4300/x86/rjump.o \
		r4300/x86/gbc.o

HEADER		=graphic/graphic_functions.h \
		main/rom.h \
		r4300/r4300.h \
		r4300/ops.h \
		r4300/macros.h \
		r4300/exception.h \
		memory/memory.h \
		memory/tlb.h \
		memory/dma.h \
		r4300/interupt.h \
		rsp/rsp.h \
		rsp/macros.h \
		rsp/ops.h \
		r4300/recomp.h \
		memory/pif.h \
		graphic/input.h \
		sound/sound.h \
		rdp/rdp.h

#LIB		=-static -lz -lc -lc_p
LIB		=-lz

all:	mupen64_sdl

graphic/sdl/g_sdl.o:		graphic/sdl/g_sdl.c
				$(CC) $(GL_PATH) $(SDL_PATH) -c -o graphic/sdl/g_sdl.o graphic/sdl/g_sdl.c

main/main.o:	main/main.c
		$(CC) -c -o main/main.o main/main.c

graphic/soft/mmx.o:		graphic/soft/mmx.asm
				nasm -f elf graphic/soft/mmx.asm -o $@

main/main_sdl.o:	main/main.c
			$(CC) -DSDL -c -o main/main_sdl.o main/main.c

mupen64_sdl:	$(OBJ) $(OBJ_SDL) $(OBJ_X86) $(HEADER) main/main_sdl.o
		$(CC) $(OBJ) $(OBJ_SDL) $(OBJ_X86) $(LIB) main/main_sdl.o -L/usr/X11R6/lib -lSDL -lGL -o mupen64_sdl
		strip --strip-all mupen64_sdl

clean:
	find . -name '*.o' -print0 | xargs -0r rm -f
	rm mupen64_glide mupen64_mesa mupen64_sdl mupen64_soft
	
clean_jed:
	find . -name '*~' -print0 | xargs -0r rm -f

gprof:
	gprof mupen64_soft > stat.txt

rdp/rdp.o:	rdp/rdp.c rdp/hle.c rdp/audio.c

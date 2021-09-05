.PHONY: all clean

EMU = emu2/emu2
CC = $(EMU) tc/tcc.exe
CFLAGS = -I\\build
TARGET = build/loadmod.exe
FILES =
FILES += debug.c
FILES += loadmod.c
FILES += zip.c

HEADERS =
HEADERS += debug.h
HEADERS += types.h
HEADERS += zip.h

OBJECTS = $(patsubst %.c,build/%.obj,$(FILES))
FILES_CRLF = $(patsubst %.c,build/%.c,$(FILES))
HEADERS_CRLF = $(patsubst %.h,build/%.h,$(HEADERS))

# What to do when the user runs 'make' with no arguments.
all: $(TARGET)

# Remove compiler artifacts.
clean:
	rm -f $(OBJECTS) $(TARGET) $(FILES_CRLF) $(HEADERS_CRLF)

# Remove everything including the compiled emulator, returning the tree to
# pristine state.
distclean: clean
	rm -rf emu2

# List dependencies
build/debug.c: build/debug.h
build/loadmod.c: build/debug.h build/types.h build/zip.h
build/zip.c: build/types.h build/zip.h

# This way is more UNIX-style but it means we have to figure out what libraries
# we need ourselves.

LD = $(EMU) tc/tlink.exe
LDFLAGS = /c/x C:\\TC\\LIB\\c0s.obj
LIBS =
#LIBS += C:\\TC\\LIB\\emu.lib
#LIBS += C:\\TC\\LIB\\maths.lib
LIBS += C:\\TC\\LIB\\cs.lib

# How to compile each source file into an object.  $(EMU) ensures the DOS
# emulator is available first.
build/%.obj: build/%.c $(EMU)
	$(CC) $(CFLAGS) -c -o$(subst /,\\,$@) $(subst /,\\,$<)

# Convert source files to DOS CRLF required by Turbo C
build/%.c: src/%.c
	unix2dos -n $< $@
build/%.h: src/%.h
	unix2dos -n $< $@

# How to build the final .exe from all the object files.
$(TARGET): $(OBJECTS)
	$(LD) $(LDFLAGS) $(subst /,\\,$^) , $(subst /,\\,$@) , , $(LIBS)
	echo -n "Hello!  This file is in .zip format, use your favourite unzipper to extract :)" | dd conv=notrunc of=$@ bs=1 seek=64

# This way is less UNIXy but it figures out what libraries we need
# automatically.

# Have to set the working directory so tcc.exe can find tlink.exe.
#EMU=EMU2_CWD="C:\\TC" emu2/emu2

# Need the emulator available to compile anything.
#src/loadmod.c: emu2/emu2

#src/loadmod.exe: src/loadmod.c
#	$(CC) $(CFLAGS) -e\\$(subst /,\\,$@) \\$(subst /,\\,$^)
#	@rm -f 'turboc.$$ln'

# Handle the DOS emulator.

# Download emu2 code.
emu2:
	git clone git@github.com:dmsc/emu2.git

# Compile emu2 DOS emulator.
emu2/emu2: emu2
	cd emu2 && $(MAKE)

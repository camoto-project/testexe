.PHONY: all clean

EMU = emu2/emu2
CC = $(EMU) tc/tcc.exe
CFLAGS = -I\\src
OBJECTS =
OBJECTS += src/debug.obj
OBJECTS += src/loadmod.obj
OBJECTS += src/zip.obj
TARGET = src/loadmod.exe

# What to do when the user runs 'make' with no arguments.
all: $(TARGET)

# Remove compiler artifacts.
clean:
	rm -f $(OBJECTS) $(TARGET)

# Remove everything including the compiled emulator, returning the tree to
# pristine state.
distclean: clean
	rm -rf emu2

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
src/%.obj: src/%.c $(EMU)
	$(CC) $(CFLAGS) -c -o$(subst /,\\,$@) $(subst /,\\,$<)

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

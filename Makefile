.PHONY: all clean distclean

# This rule has to be first so that a plain `make` calls it, but we can't
# declare it until we've specified the TARGETS_X rules.
all:

EMU = emu2/emu2
# Have to set the working directory so tcc.exe can find tlink.exe.
CC = EMU2_CWD="C:\\TC" $(EMU) tc/tcc.exe
CFLAGS = -I\\build -f- -k

# These are the PKLite versions to compress with.
VERSIONS = 100 103 105 112 112r 113 113r 114 115 115r 150 201

TARGETS_T = $(addsuffix .exe,$(addprefix compress/t,$(VERSIONS)))
TARGETS_H = $(addsuffix .exe,$(addprefix compress/h,$(VERSIONS)))

# Don't delete these intermediate files unless the build fails.
build/test-h.exe:
build/test-t.exe:

# Remove everything including the compiled emulator, returning the tree to
# pristine state.
distclean: clean
	rm -rf emu2

# Need the emulator available to compile anything.
src/test-s.c: emu2/emu2

# Don't seem to be able to run multiple emu2 instances with Turbo C at the same
# time, although PKLite is fine.
.NOTPARALLEL: build/test-%.exe

# Compile the test .exe using whatever memory model is in the filename.
build/test-%.exe: src/test-%.c
	$(CC) $(CFLAGS) -m$* -e\\$(subst /,\\,$@) \\$(subst /,\\,$^)
	@rm -f 'turboc.$$ln' tc/$(@F:%.exe=%.obj)
	@if [ ! -f "$@" ]; then echo Failed to create "$@"; false; fi

# Compress each file with large mode off.
compress/t%.exe: build/test-t.exe
	cp "$<" "$@"
	@echo -en "\033[1;34m"
	if [ "$(findstring r,$*)" == "r" ]; then OPTS="-e"; fi && \
	EMU2_DRIVE_C=pklite/ EMU2_DRIVE_D=compress/ $(EMU) pklite/pkl$*.exe $$OPTS d:\\$(@F)
	@echo -en "\033[0m"

# Compress each file with large mode on.  It would be nice to avoid
# duplicating this rule, but there's no neat way to do it.
compress/h%.exe: build/test-h.exe
	cp "$<" "$@"
	@echo -en "\033[1;34m"
	if [ "$(findstring r,$*)" == "r" ]; then OPTS="-e"; fi && \
	EMU2_DRIVE_C=pklite/ EMU2_DRIVE_D=compress/ $(EMU) pklite/pkl$*.exe $$OPTS d:\\$(@F)
	@echo -en "\033[0m"

# What to do when the user runs 'make' with no arguments.
all: $(TARGETS_T) $(TARGETS_H)

# Remove compiler artifacts.
clean:
	rm -f $(TARGETS_T) $(TARGETS_H) build/*.exe

# Handle the DOS emulator.

# Download emu2 code.
emu2:
	git clone git@github.com:dmsc/emu2.git

# Compile emu2 DOS emulator.
emu2/emu2: emu2
	cd emu2 && $(MAKE)

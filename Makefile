.PHONY: all clean distclean

# This rule has to be first so that a plain `make` calls it, but we can't
# declare it until we've specified the TARGETS_X rules.
all:

EMU = emu2/emu2
# Have to set the working directory so tcc.exe can find tlink.exe.
CC = EMU2_CWD="C:\\TC" $(EMU) tc/tcc.exe
CFLAGS = -I\\build -n\\build -f- -k

# These are the PKLite versions to compress with.  They are in hex to match the
# codes in the .exe header, so 10e -> 1,0e -> 1.14.
VERSIONS = 100 103 105 10c 10d 10e 10f 132 201
VERSIONS_R = 10cr 10dr 10fr

TARGETS_T = $(addsuffix .exe,$(addprefix compress/t,$(VERSIONS) $(VERSIONS_R))) \
	$(addsuffix .unc,$(addprefix compress/t,$(VERSIONS_R))) \
	compress/t.unc

TARGETS_H = $(addsuffix .exe,$(addprefix compress/h,$(VERSIONS) $(VERSIONS_R))) \
	$(addsuffix .unc,$(addprefix compress/h,$(VERSIONS_R))) \
	compress/h.unc

# Don't delete these intermediate files unless the build fails.
build/test-h.exe:
build/test-t.exe:
build/packhdr.exe:

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
	$(CC) $(CFLAGS) -m$* -e$(@F) \\$(subst /,\\,$^)
	@rm -f 'turboc.$$ln'
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

build/%.exe: src/%.c
	$(CC) $(CFLAGS) -mt -e$(@F) \\$(subst /,\\,$^)
	@rm -f 'turboc.$$ln'
	@if [ ! -f "$@" ]; then echo Failed to create "$@"; false; fi

# Pack the header for each output .exe.  This should result in a file with the
# same content as PKLite produces (after decompression) with the -e flag, where
# it discards extra .exe header data and normalises the relocation table.
compress/h%r.unc: build/test-h.exe build/packhdr.exe
	$(EMU) build/packhdr.exe -e 3$* < "$<" > "$@"

compress/t%r.unc: build/test-t.exe build/packhdr.exe
	$(EMU) build/packhdr.exe -e 1$* < "$<" > "$@"

compress/%.unc: build/test-%.exe
	cp "$<" "$@"

# What to do when the user runs 'make' with no arguments.
all: $(TARGETS_T) $(TARGETS_H) build/packhdr.exe

# Remove compiler artifacts.
clean:
	rm -f $(TARGETS_T) $(TARGETS_H) build/*.exe build/*.obj

# Handle the DOS emulator.

# Download emu2 code.
emu2:
	git clone git@github.com:dmsc/emu2.git

# Compile emu2 DOS emulator.
emu2/emu2: emu2
	cd emu2 && $(MAKE)

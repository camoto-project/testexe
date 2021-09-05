# loadmod
Copyright 2010-2021 Adam Nielsen <<malvineous@shikadi.net>>  

This program is inspired by, and based upon, Admiral Bob's ckpatch.

It loads a DOS .exe file into memory, decompresses it if required, patches it,
then runs it.

By only changing the data in memory once the .exe file has been loaded, it
allows modifying games without changing any of the original game files.

This means the original game files do not need to be distributed with a user
modification (so there are no piracy issues to contend with) and modified
versions of the original game files do not need to be distributed either (which
is against most end-user licence agreements).

This repository includes a partial copy for Borland Turbo C v2.01, which is
copyright 1987-1988 Borland International.  This compiler has been graciously
released as freeware by Borland and the full version including the IDE is
available at https://archive.org/details/msdos_borland_turbo_c_2.01

This compiler is used to build the code and produce a lean DOS .exe file that
can run in real-mode on an 8086 if required.

The [EMU2](https://github.com/dmsc/emu2) text-mode x86 + DOS emulator is used
to run Turbo C.  A copy is downloaded and built automatically as part of the
compilation process.  The CLI nature of the emulator means it is well suited to
be part of a Makefile-based build process running natively on the host.

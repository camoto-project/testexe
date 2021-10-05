# testexe

This repo builds a sample DOS .exe file and compresses it with various
utilities.

It is intended to produce small files suitable for unit testing other code.
[gamecomp.js](https://github.com/Malvineous/gamecompjs) uses these files to
confirm that the PKLite algorithm is correctly decompressing files, for example.

If you wish to use the files in your own test suite, you can simply copy them
without running the code here.  The uncompressed files are in the `build/`
folder, and the compressed files are in the `compress/` folder.

* `compress/t*.exe` - PKLite compressed `build/test-t.exe` ("large" flag off)
* `compress/h*.exe` - PKLite compressed `build/test-h.exe` ("large" flag on)
* `compress/[th]*r.exe` - PKLite 'extra' flag used (to prevent decompression)

This repository includes a partial copy for Borland Turbo C v2.01, which is
copyright 1987-1988 Borland International.  This compiler has been graciously
released as freeware by Borland and the full version including the IDE is
available at https://archive.org/details/msdos_borland_turbo_c_2.01

This repository also includes a number of different versions of PKLite,
including registered versions.  PKWare does not seem to sell this product any
longer and the files can be found freely online, so I am hoping it won't cause
any problems by including them in this archive.

The [EMU2](https://github.com/dmsc/emu2) text-mode x86 + DOS emulator is used
to run Turbo C.  A copy is downloaded and built automatically as part of the
compilation process.  The CLI nature of the emulator means it is well suited to
being part of a Makefile-based build process running natively on the host.

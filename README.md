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
* `compress/[th]*r.unc` - Decompressed versions with normalised relocation
  tables.  This is because PKLite with the 'extra' flag rewrites the relocation
  table to compress more efficiently, so upon decompression it cannot be
  restored back to the original values.  This means comparing the decompressed
  data to `build/test-[th].exe` will fail due to the different relocation
  table.  Instead, the original uncompressed .exe has its relocation table
  rewritten in the same way PKLite does, and this is stored as the `.unc` file.
  This means for files that have an `.unc` present, it should be used instead
  of `build/test-[th].exe` when comparing the decompressor output to the
  original `.exe`.

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

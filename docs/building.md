# Building filesysbox

## Language standard

filesysbox requires a C99-capable compiler.

This is assumed throughout the codebase (e.g. use of `long long` / 64-bit integer types and related constructs).
When building with vbcc, make sure C99 mode is enabled.

## Compiler notes

### m68k-amigaos

Currently, the only officially supported compiler for building filesysbox on m68k-amigaos is GCC.
The maintainer recommends bebbo's GCC 6.5.0b (this is what is used for releases).

### vbcc

Building with vbcc is not officially supported at this time.
A dedicated vbcc build setup (ideally a makefile.vbcc) would be needed before it can be recommended.

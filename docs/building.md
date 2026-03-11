# Building filesysbox

## Language standard

filesysbox requires a C99-capable compiler.

This is assumed throughout the codebase (e.g. use of `long long` / 64-bit integer types and related constructs).
When building with vbcc, make sure C99 mode is enabled.

## Compiler notes

### Build assumptions

The project build defines `__NOLIBBASE__` in the project makefiles.

This affects how the Amiga `proto/` headers behave: global library base declarations are disabled, so explicit library base declarations in the source tree are intentional in some places.

Contributors working on portability, compiler-compatibility, or cleanup changes should keep this in mind before treating explicit `extern` library base declarations as redundant.

### m68k-amigaos

Currently, the only officially supported compiler for building filesysbox on m68k-amigaos is GCC.
The maintainer recommends bebbo's GCC 6.5.0b (this is what is used for releases).

### vbcc

Building with vbcc is not officially supported at this time.
A dedicated vbcc build setup (for example a makefile.vbcc) would be needed before it can be recommended.

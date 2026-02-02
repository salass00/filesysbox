# Building filesysbox

## Language standard

filesysbox requires a C99-capable compiler.

This is assumed throughout the codebase (e.g. use of `long long` / 64-bit integer types and related constructs).
When building with vbcc, make sure C99 mode is enabled.

## Compiler notes

### vbcc (m68k)

Use `-c99` when compiling C sources, for example:

- Add `-c99` to your `CFLAGS`
- Or compile with: `vc -c99 ...`

### GCC / Clang

GCC/Clang default settings are typically sufficient, but ensure your build uses a C99 (or later) mode if you override the standard.


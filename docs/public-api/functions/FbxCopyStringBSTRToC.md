# FbxCopyStringBSTRToC

> Status: working draft
> Sources: public headers, implementation review
> Goal: explicit documentation of the public string-conversion contract of `FbxCopyStringBSTRToC()`

## Purpose

`FbxCopyStringBSTRToC()` copies a BCPL or BSTR-style string into a caller-supplied C string buffer.

It is a small public helper function for converting Amiga-style BSTR data into normal C string form.

## Synopsis

```
void FbxCopyStringBSTRToC(BSTR src, STRPTR dst, ULONG size);
```

## Public role in the lifecycle

`FbxCopyStringBSTRToC()` is not a lifecycle transition.

It does not:

* create an instance
* begin runtime
* destroy an instance

It is a helper function in the public string-conversion surface.

## Inputs

`FbxCopyStringBSTRToC()` takes:

* `src`
* `dst`
* `size`

### `src`

`src` is the source BSTR.

### `dst`

`dst` is the caller-supplied destination C string buffer.

### `size`

`size` is the size of the destination buffer in bytes.

This is part of the public buffer contract of the function.

## Result

`FbxCopyStringBSTRToC()` does not return a result value.

Its public effect is to write the converted C string representation into the caller-supplied destination buffer.

## Buffer contract

`FbxCopyStringBSTRToC()` uses caller-owned destination storage.

That means:

* filesysbox does not allocate the output buffer
* filesysbox does not take ownership of the output buffer
* the caller must provide valid writable storage through `dst`
* the `size` argument defines the size of that storage for the conversion operation

This is a standard caller-supplied buffer helper contract.

## Conversion role

This function exists to bridge two string representations:

* Amiga-style BSTR input
* C string output

Its role is representation conversion, not storage ownership management and not lifecycle control.

## Relationship to `FbxCopyStringCToBSTR()`

`FbxCopyStringBSTRToC()` and `FbxCopyStringCToBSTR()` form the public string-conversion pair.

The distinction is:

* `FbxCopyStringBSTRToC()` converts from BSTR to C string
* `FbxCopyStringCToBSTR()` converts from C string to BSTR

Both belong to the same helper surface of the API.

## Instance requirements

Unlike many other helper functions, `FbxCopyStringBSTRToC()` does not take a `struct FbxFS *` instance argument.

That means it is a library-level helper rather than an instance-bound helper.

Its use does not depend on instance lifetime in the same way that `FbxGetSysTime()` or `FbxGetUpTime()` do.

## Side effects

`FbxCopyStringBSTRToC()` has one direct side effect:

* writing to the caller-supplied destination buffer

It otherwise behaves as a pure representation-conversion helper.

## Minimal backend relevance

A minimal backend does not need `FbxCopyStringBSTRToC()` in order to mount and run normally.

This function belongs to the helper layer of the public API.

Its importance is therefore:

* API completeness
* string conversion at the Amiga/C boundary
* support for helpers, wrappers, or tooling that must move between BSTR and C string representations

## Practical rules for callers and backend authors

A caller or backend author should follow these rules:

* provide valid source BSTR input
* provide valid writable destination storage through `dst`
* pass the actual destination buffer size through `size`
* treat the function as a representation-conversion helper, not as a lifecycle or ownership-transfer function

## Relationship to other public contracts

`FbxCopyStringBSTRToC()` must be read together with:

* `FbxCopyStringCToBSTR()` as the inverse public string-conversion helper
* the broader public helper layer of the filesysbox API

## Notes

This document describes the string-conversion contract already made explicit through:

* the public prototype of `FbxCopyStringBSTRToC()`
* its caller-supplied buffer arguments
* its role as the BSTR-to-C half of the public string-conversion helper pair


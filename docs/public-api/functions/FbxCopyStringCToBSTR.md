# FbxCopyStringCToBSTR

> Status: working draft
> Sources: public headers, implementation review
> Goal: explicit documentation of the public string-conversion contract of `FbxCopyStringCToBSTR()`

## Purpose

`FbxCopyStringCToBSTR()` copies a C string into a caller-supplied BCPL or BSTR-style destination.

It is a small public helper function for converting normal C string data into Amiga-style BSTR form.

## Synopsis

```
void FbxCopyStringCToBSTR(CONST_STRPTR src, BSTR dst, ULONG size);
```

## Public role in the lifecycle

`FbxCopyStringCToBSTR()` is not a lifecycle transition.

It does not:

* create an instance
* begin runtime
* destroy an instance

It is a helper function in the public string-conversion surface.

## Inputs

`FbxCopyStringCToBSTR()` takes:

* `src`
* `dst`
* `size`

### `src`

`src` is the source C string.

### `dst`

`dst` is the caller-supplied destination BSTR buffer.

### `size`

`size` is the size or capacity of the destination storage used for the conversion operation.

This is part of the public buffer contract of the function.

## Result

`FbxCopyStringCToBSTR()` does not return a result value.

Its public effect is to write the converted BSTR representation into the caller-supplied destination buffer.

## Buffer contract

`FbxCopyStringCToBSTR()` uses caller-owned destination storage.

That means:

* filesysbox does not allocate the output buffer
* filesysbox does not take ownership of the output buffer
* the caller must provide valid writable storage through `dst`
* the `size` argument defines the size or capacity of that storage for the conversion operation

This is a standard caller-supplied buffer helper contract.

## Conversion role

This function exists to bridge two string representations:

* C string input
* Amiga-style BSTR output

Its role is representation conversion, not storage ownership management and not lifecycle control.

## Relationship to `FbxCopyStringBSTRToC()`

`FbxCopyStringCToBSTR()` and `FbxCopyStringBSTRToC()` form the public string-conversion pair.

The distinction is:

* `FbxCopyStringCToBSTR()` converts from C string to BSTR
* `FbxCopyStringBSTRToC()` converts from BSTR to C string

Both belong to the same helper surface of the API.

## Instance requirements

Unlike many other helper functions, `FbxCopyStringCToBSTR()` does not take a `struct FbxFS *` instance argument.

That means it is a library-level helper rather than an instance-bound helper.

Its use does not depend on instance lifetime in the same way that `FbxGetSysTime()` or `FbxGetUpTime()` do.

## Side effects

`FbxCopyStringCToBSTR()` has one direct side effect:

* writing to the caller-supplied destination buffer

It otherwise behaves as a pure representation-conversion helper.

## Minimal backend relevance

A minimal backend does not need `FbxCopyStringCToBSTR()` in order to mount and run normally.

This function belongs to the helper layer of the public API.

Its importance is therefore:

* API completeness
* string conversion at the C/Amiga boundary
* support for helpers, wrappers, or tooling that must move between C strings and BSTR representations

## Practical rules for callers and backend authors

A caller or backend author should follow these rules:

* provide valid source C string input
* provide valid writable destination storage through `dst`
* pass the actual destination capacity through `size`
* treat the function as a representation-conversion helper, not as a lifecycle or ownership-transfer function

## Relationship to other public contracts

`FbxCopyStringCToBSTR()` must be read together with:

* `FbxCopyStringBSTRToC()` as the inverse public string-conversion helper
* the broader public helper layer of the filesysbox API

## Notes

This document describes the string-conversion contract already made explicit through:

* the public prototype of `FbxCopyStringCToBSTR()`
* its caller-supplied buffer arguments
* its role as the C-to-BSTR half of the public string-conversion helper pair


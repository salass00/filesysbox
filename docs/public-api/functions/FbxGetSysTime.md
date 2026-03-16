# FbxGetSysTime

> Status: working draft
> Sources: public headers, implementation review
> Goal: explicit documentation of the public helper contract of `FbxGetSysTime()`

## Purpose

`FbxGetSysTime()` writes the current system time into a caller-supplied `struct timeval`.

It is a small public helper function for retrieving current time information through the filesysbox API.

## Synopsis

```
void FbxGetSysTime(struct FbxFS * fs, struct timeval * tv);
```

## Public role in the lifecycle

`FbxGetSysTime()` is not a lifecycle transition.

It does not:

* create an instance
* begin runtime
* destroy an instance

It is an instance-bound helper function.

## Inputs

`FbxGetSysTime()` takes:

* `fs`
* `tv`

### `fs`

`fs` is the filesysbox instance whose helper interface is being used.

For the practical public contract, it must be a live instance created by `FbxSetupFS()`.

### `tv`

`tv` is a caller-supplied `struct timeval` output buffer.

The function writes the current system time into this structure.

## Result

`FbxGetSysTime()` does not return a result value.

Its public effect is to store the current system time in the caller-supplied `struct timeval`.

## Output-buffer contract

The caller owns the output buffer supplied through `tv`.

That means:

* filesysbox does not allocate the buffer
* filesysbox does not retain ownership of it
* the caller must provide valid writable storage

This is a simple output-buffer helper contract.

## Relationship to `FbxGetUpTime()`

`FbxGetSysTime()` and `FbxGetUpTime()` are related helper functions, but they report different kinds of time values.

The practical distinction is:

* `FbxGetSysTime()` reports current system time
* `FbxGetUpTime()` reports uptime-style or elapsed runtime time

That difference is part of the public helper surface.

## Instance requirements

`FbxGetSysTime()` operates on an existing filesysbox instance.

The practical rule is:

* call it only on an instance created by `FbxSetupFS()`
* do not treat it as a pre-setup function
* do not treat it as a startup-message helper

## Side effects

`FbxGetSysTime()` is a helper query with one direct side effect:

* writing to the caller-supplied `struct timeval`

It does not otherwise change lifecycle state.

## Minimal backend relevance

A minimal backend does not need `FbxGetSysTime()` in order to mount and run normally.

This function belongs to the smaller public helper surface rather than to the core setup/runtime/teardown path.

Its importance is therefore:

* API completeness
* helper and diagnostic use
* support for tools or wrappers that want time information through the filesysbox API

## Practical rules for callers and backend authors

A caller or backend author should follow these rules:

* call `FbxGetSysTime()` only on a live filesysbox instance
* provide valid writable `struct timeval` storage
* treat the function as a helper query, not a lifecycle transition

## Relationship to other public contracts

`FbxGetSysTime()` must be read together with:

* `FbxGetUpTime()` as the related time helper
* `FbxSetupFS()` because it requires an existing instance
* `FbxCleanupFS()` because helper use cannot outlive instance lifetime

## Notes

This document describes the helper contract already made explicit through:

* the public prototype of `FbxGetSysTime()`
* the caller-supplied `struct timeval` output model
* the distinction between system time and uptime-style time in the public helper surface


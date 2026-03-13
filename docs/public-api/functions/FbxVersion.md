# FbxVersion

> Status: working draft
> Sources: public headers, implementation review
> Goal: explicit documentation of the public version-query contract of `FbxVersion()`

## Purpose

`FbxVersion()` returns the filesysbox library version.

It is a small public query function for identifying the library version exposed by the installed filesysbox API.

## Synopsis

```
LONG FbxVersion(void);
```

## Public role in the lifecycle

`FbxVersion()` is not a lifecycle transition.

It does not:

* create an instance
* begin runtime
* destroy an instance

It is a library-level query function.

## Input

`FbxVersion()` takes no public input.

This is part of its public simplicity: it is a direct version query.

## Result

`FbxVersion()` returns a `LONG` representing the filesysbox library version.

Its public role is to expose the version identity of the installed library.

## Library-level query model

`FbxVersion()` is a library-level query, not an instance-level query.

That means:

* it does not require `struct FbxFS *`
* it does not depend on setup or runtime state
* it applies to the library itself rather than to one mounted filesystem instance

This distinguishes it clearly from instance-bound functions such as `FbxQueryFS()`, `FbxGetSysTime()`, or `FbxGetUpTime()`.

## Relationship to `FbxFuseVersion()`

`FbxVersion()` and `FbxFuseVersion()` are both small public version-query functions, but they report different things.

The distinction is:

* `FbxVersion()` reports the filesysbox library version
* `FbxFuseVersion()` reports the FUSE-style compatibility or API version exposed by filesysbox

Both belong to the public version-query surface.

## Side effects

`FbxVersion()` should be treated as side-effect free.

It does not:

* allocate or destroy instance state
* depend on mounted runtime state
* alter library state through normal use

## Minimal backend relevance

A minimal backend does not need `FbxVersion()` in order to mount and run normally.

This function belongs to the smaller public query surface rather than to the core setup/runtime/teardown path.

Its importance is therefore:

* API completeness
* diagnostics
* version reporting
* compatibility checks in wrappers or supporting tools

## Practical rules for callers and backend authors

A caller or backend author should follow these rules:

* use `FbxVersion()` when the library version itself is needed
* do not confuse it with instance-level state or runtime state
* distinguish it from `FbxFuseVersion()` when FUSE-style API compatibility is the real question

## Relationship to other public contracts

`FbxVersion()` must be read together with:

* `FbxFuseVersion()` as the related public version query
* the broader public API because it identifies the library version of that API surface

## Notes

This document describes the version-query contract already made explicit through:

* the public prototype of `FbxVersion()`
* the distinction between library version and FUSE-style compatibility version in the public API surface


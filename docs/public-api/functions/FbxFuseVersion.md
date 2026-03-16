# FbxFuseVersion

> Status: working draft
> Sources: public headers, implementation review
> Goal: explicit documentation of the public version-query contract of `FbxFuseVersion()`

## Purpose

`FbxFuseVersion()` returns the FUSE-style compatibility or API version exposed by filesysbox.

It is a small public query function for identifying which FUSE-style API level the installed filesysbox library presents.

## Synopsis

```
LONG FbxFuseVersion(void);
```

## Public role in the lifecycle

`FbxFuseVersion()` is not a lifecycle transition.

It does not:

* create an instance
* begin runtime
* destroy an instance

It is a library-level query function.

## Input

`FbxFuseVersion()` takes no public input.

This is part of its public simplicity: it is a direct version query.

## Result

`FbxFuseVersion()` returns a `LONG` representing the FUSE-style compatibility or API version exposed by filesysbox.

Its public role is to expose the compatibility level of the filesysbox FUSE-style API surface.

## Library-level query model

`FbxFuseVersion()` is a library-level query, not an instance-level query.

That means:

* it does not require `struct FbxFS *`
* it does not depend on setup or runtime state
* it applies to the library’s exposed FUSE-style API surface rather than to one mounted filesystem instance

This distinguishes it clearly from instance-bound functions such as `FbxQueryFS()`, `FbxGetSysTime()`, or `FbxGetUpTime()`.

## Relationship to `FbxVersion()`

`FbxFuseVersion()` and `FbxVersion()` are both small public version-query functions, but they report different things.

The distinction is:

* `FbxFuseVersion()` reports the FUSE-style compatibility or API version
* `FbxVersion()` reports the filesysbox library version

Both belong to the public version-query surface.

## Side effects

`FbxFuseVersion()` should be treated as side-effect free.

It does not:

* allocate or destroy instance state
* depend on mounted runtime state
* alter library state through normal use

## Minimal backend relevance

A minimal backend does not need `FbxFuseVersion()` in order to mount and run normally.

This function belongs to the smaller public query surface rather than to the core setup/runtime/teardown path.

Its importance is therefore:

* API completeness
* diagnostics
* compatibility checks in wrappers or supporting tools
* identification of the FUSE-style API level expected by higher-level code

## Practical rules for callers and backend authors

A caller or backend author should follow these rules:

* use `FbxFuseVersion()` when the FUSE-style API compatibility level is needed
* do not confuse it with the library version returned by `FbxVersion()`
* do not treat it as an instance-level query

## Relationship to other public contracts

`FbxFuseVersion()` must be read together with:

* `FbxVersion()` as the related library-version query
* the broader public API because it identifies the exposed FUSE-style compatibility surface of that API

## Notes

This document describes the version-query contract already made explicit through:

* the public prototype of `FbxFuseVersion()`
* the distinction between filesysbox library version and FUSE-style compatibility version in the public API surface


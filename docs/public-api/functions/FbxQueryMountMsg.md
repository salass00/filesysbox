# FbxQueryMountMsg

> Status: working draft
> Sources: public headers, implementation review
> Goal: explicit documentation of the public startup-query contract of `FbxQueryMountMsg()`

## Purpose

`FbxQueryMountMsg()` inspects a filesysbox startup or mount message and returns selected information from it.

It is a read-only helper function used during the startup phase, before the normal mounted runtime begins.

Its purpose is to let a caller inspect startup context without yet creating a filesysbox instance.

## Synopsis

```
APTR FbxQueryMountMsg(struct Message * msg, LONG attr);
```

## Public role in the lifecycle

`FbxQueryMountMsg()` belongs to the startup-message phase of the public lifecycle.

It may be used before `FbxSetupFS()` to inspect startup context and decide how setup should proceed.

It does not:

* create a filesysbox instance
* begin runtime
* reply to the startup message

That makes it a pure startup-query helper.

## Inputs

`FbxQueryMountMsg()` takes two public inputs:

* `msg`
* `attr`

### `msg`

`msg` is a startup or mount message.

The function interprets this message as the source of startup context.

### `attr`

`attr` selects which startup-context value should be returned.

The currently documented public selectors are:

* `FBXQMM_MOUNT_NAME`
* `FBXQMM_MOUNT_CONTROL`
* `FBXQMM_FSSM`
* `FBXQMM_ENVIRON`

These selectors define the currently documented public query surface of this function.

## Result

`FbxQueryMountMsg()` returns an `APTR`.

The meaning of the returned pointer depends on the selector supplied through `attr`.

If the requested value is not available, the function returns `NULL`.

This makes the result contract simple:

* return the requested startup-context pointer when available
* otherwise return `NULL`

## Borrowed startup-context data

The pointer returned by `FbxQueryMountMsg()` is borrowed startup-context data.

The caller does not own it.

That means:

* the caller must not free it
* the caller must not treat it as caller-owned storage
* the caller must not assume it remains valid after startup-message handling has been completed

If the caller needs startup information beyond the startup phase, it should copy that information.

## Meaning of the documented selectors

### `FBXQMM_MOUNT_NAME`

This selector returns the mount name associated with the startup context.

### `FBXQMM_MOUNT_CONTROL`

This selector returns the mount-control structure associated with the startup context.

### `FBXQMM_FSSM`

This selector returns the filesystem startup message associated with the startup context.

### `FBXQMM_ENVIRON`

This selector returns the environment vector associated with the startup context.

These are the currently documented public startup-query values.

## Relationship to `FbxSetupFS()`

`FbxQueryMountMsg()` is typically used before `FbxSetupFS()`.

The practical pattern is:

1. inspect startup context through `FbxQueryMountMsg()`
2. decide how setup should proceed
3. call `FbxSetupFS()` if normal setup should continue

When `FbxSetupFS()` is called with a valid startup message, filesysbox handles the reply path for that message itself.

That means `FbxQueryMountMsg()` is an inspection step, not a completion step.

## Relationship to `FbxReturnMountMsg()`

`FbxQueryMountMsg()` and `FbxReturnMountMsg()` belong to the same startup-message phase, but they have different roles.

* `FbxQueryMountMsg()` inspects startup context
* `FbxReturnMountMsg()` explicitly completes the reply path

This distinction is part of the public lifecycle.

## Relationship to instance lifetime

`FbxQueryMountMsg()` does not create a `struct FbxFS *` instance.

That means:

* there is no filesysbox instance lifetime yet
* the function operates entirely before setup has created a runtime object

It belongs to pre-instance control flow.

## Minimal backend usage

A minimal backend may use `FbxQueryMountMsg()` if it needs startup information before calling `FbxSetupFS()`.

A minimal backend does not need to use it if setup can proceed without inspecting startup context explicitly.

This function is therefore optional in the normal minimal lifecycle, but fully valid when startup inspection is needed.

## Practical rules for callers and backend authors

A caller or backend author should follow these rules:

* use `FbxQueryMountMsg()` only for startup-message inspection
* treat returned pointers as borrowed startup-context data
* copy startup data if it must outlive the startup phase
* do not confuse `FbxQueryMountMsg()` with instance creation or startup-message reply handling

## Relationship to other public contracts

`FbxQueryMountMsg()` must be read together with:

* `FbxReturnMountMsg()` for explicit startup-message reply handling
* `FbxSetupFS()` for normal setup that consumes the startup path
* the lifecycle document for the full startup → setup → runtime sequence

## Notes

This document describes the startup-query contract already made explicit through:

* the public prototype of `FbxQueryMountMsg()`
* the documented selector constants
* the role of startup-message inspection in the public lifecycle

It distinguishes deliberately between:

* borrowed startup-context data
* explicit startup-message reply handling
* later instance creation through `FbxSetupFS()`


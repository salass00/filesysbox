# Public lifecycle

> Status: working draft
> Sources: public headers, implementation review
> Goal: explicit documentation of the public lifecycle of a filesysbox-managed filesystem instance

## Purpose

This document describes the public lifecycle of a filesysbox-managed filesystem instance.

The lifecycle is built around these public functions:

* `FbxQueryMountMsg()`
* `FbxReturnMountMsg()`
* `FbxSetupFS()`
* `FbxEventLoop()`
* `FbxCleanupFS()`

The purpose of this document is to state explicitly:

* how startup-message handling fits into setup
* what the normal call order is
* where runtime begins and ends
* what ownership and lifetime rules apply to the main public inputs and outputs
* how failure during setup differs from later runtime termination

## Public lifecycle overview

The normal public lifecycle is:

1. optionally inspect startup context with `FbxQueryMountMsg()`
2. call `FbxSetupFS()`
3. if setup succeeds, call `FbxEventLoop()`
4. when the event loop returns, call `FbxCleanupFS()`

This is the standard lifecycle for a backend that is mounted and then allowed to run normally.

The lifecycle therefore has three main phases:

* setup
* runtime
* teardown

A startup-message inspection or reply path may occur before or during setup, depending on how the caller uses the public API.

## Startup-message phase

### Role of `FbxQueryMountMsg()`

`FbxQueryMountMsg()` is a read-only startup helper.

It allows the caller to inspect information carried by the startup or mount message before deciding how setup should proceed.

The currently documented public selectors are:

* `FBXQMM_MOUNT_NAME`
* `FBXQMM_MOUNT_CONTROL`
* `FBXQMM_FSSM`
* `FBXQMM_ENVIRON`

This function does not create a filesystem instance and does not begin runtime.

### Role of `FbxReturnMountMsg()`

`FbxReturnMountMsg()` explicitly replies to the startup or mount message.

It exists for control paths where the caller needs to complete startup-message handling directly instead of letting `FbxSetupFS()` do that work.

This function also does not create a filesystem instance and does not begin runtime.

### Ownership of queried startup data

Pointers returned by `FbxQueryMountMsg()` are borrowed views into startup context.

The caller does not own them and must not free them.

They should not be assumed valid after the startup-message path has been completed.

If startup data must outlive that phase, it should be copied by the caller.

## Setup phase

### Role of `FbxSetupFS()`

`FbxSetupFS()` is the public setup entry point.

It creates and initializes the filesysbox-managed filesystem instance.

Its public inputs are:

* `struct Message *msg`
* `const struct TagItem *tags`
* `const struct fuse_operations *ops`
* `LONG opssize`
* `APTR udata`

Its public result is:

* a valid `struct FbxFS *` on success
* `NULL` on failure

### Ownership during setup

#### Operation table

The supplied operation table is copied internally up to `opssize`.

This means the original `ops` object only needs to remain valid during the setup call itself.

#### Backend private context

The supplied `udata` pointer is stored, but not copied.

This means the caller or backend remains responsible for its lifetime.

The practical rule is:

* `udata` must remain valid for the intended lifetime of the mounted backend

#### Startup message

When `FbxSetupFS()` is called with a non-NULL startup message, filesysbox handles the reply path itself.

That means the normal setup path is:

* inspect startup context first if needed
* then call `FbxSetupFS()`
* do not separately call `FbxReturnMountMsg()` for the same successful setup path

### Setup-time normalization

During setup, filesysbox normalizes the supplied operation table.

That includes documented and observed fallback behavior such as:

* `opendir` falls back to `open`
* `releasedir` falls back to `release`
* `fgetattr` falls back to `getattr`
* `ftruncate` falls back to `truncate`

Setup also interprets setup tags and filesysbox-specific flags.

### Setup success

When setup succeeds:

* a valid `struct FbxFS *` exists
* backend-private context has been attached
* the normalized callback table is active
* the instance is ready for runtime through `FbxEventLoop()`

### Setup failure

When setup fails:

* `FbxSetupFS()` returns `NULL`
* the caller must not enter `FbxEventLoop()`
* no usable filesystem instance exists for later use

Reviewed setup paths show that filesysbox cleans up partially initialized internal state before returning failure.

This means setup failure is self-contained at the public boundary:

* failure is reported by returning `NULL`
* partial setup does not escape as a usable instance

## Runtime phase

### Role of `FbxEventLoop()`

`FbxEventLoop()` is the public runtime entry point.

It runs the active filesysbox processing loop for a previously created instance.

The instance passed to `FbxEventLoop()` must be a valid result from `FbxSetupFS()`.

### What runtime means

During runtime, filesysbox processes the live operation of the filesystem instance.

The reviewed runtime responsibilities include:

* filesystem packet handling
* notify reply handling
* signal-driven work
* timer-related work
* volume setup and cleanup transitions
* backend hook invocation through the installed operation table

This makes `FbxEventLoop()` the active execution phase of the mounted backend.

### Runtime ownership

During runtime:

* filesysbox owns the `struct FbxFS *` instance as its active internal object
* the caller remains responsible for eventually calling `FbxCleanupFS()` after runtime ends
* the backend remains responsible for the validity of its private context stored through `udata`

### Runtime exit

`FbxEventLoop()` returns when the active runtime phase ends.

The reviewed implementation indicates that it currently returns `0`.

For public lifecycle purposes, the important meaning is not the numeric value itself, but this transition:

* runtime has ended
* teardown must now follow through `FbxCleanupFS()`

Returning from `FbxEventLoop()` does not itself destroy the filesystem instance.

## Teardown phase

### Role of `FbxCleanupFS()`

`FbxCleanupFS()` is the public teardown entry point.

It destroys the filesysbox-managed filesystem instance and releases its internal resources.

It is the public counterpart to `FbxSetupFS()`.

### Teardown inputs

`FbxCleanupFS()` accepts:

* a valid `struct FbxFS *`
* or `NULL`

This means defensive cleanup with `FbxCleanupFS(NULL)` is permitted.

### Teardown effect

After `FbxCleanupFS()` returns for a valid instance:

* that instance must be considered dead
* it must not be used again
* runtime is over for that filesystem instance

The practical rule is:

* call `FbxCleanupFS()` once for a successfully created instance
* do not reuse the instance afterwards

## Normal public flow

The normal public flow is therefore:

1. optional startup-message inspection through `FbxQueryMountMsg()`
2. `FbxSetupFS()`
3. `FbxEventLoop()`
4. `FbxCleanupFS()`

This is the standard public lifecycle for a normal mounted backend.

## Alternate startup-message flow

The public API also supports an alternate startup-message control path.

That alternate path is:

1. inspect startup context with `FbxQueryMountMsg()` if needed
2. decide not to proceed with normal setup
3. explicitly reply through `FbxReturnMountMsg()`

This path ends before a filesysbox instance is created.

## Practical ownership summary

### Borrowed startup data

Data returned by `FbxQueryMountMsg()` is borrowed startup context.

The caller must not free it.

### Operation table

The `ops` table is copied during setup.

The caller does not need to preserve it after the setup call returns.

### Backend private context

The `udata` pointer is stored, not copied.

The backend or caller must keep it valid for the intended backend lifetime.

### Filesysbox instance

The `struct FbxFS *` returned by `FbxSetupFS()` is the public lifecycle handle of the instance.

It remains valid from successful setup until `FbxCleanupFS()` completes.

## Practical failure summary

### Before instance creation

Failure is expressed through:

* explicit startup-message reply without setup
* or `FbxSetupFS()` returning `NULL`

### After instance creation

Runtime ends through `FbxEventLoop()` returning.

Teardown then completes through `FbxCleanupFS()`.

This means setup failure and runtime termination are different public lifecycle events:

* setup failure means no usable instance ever existed
* runtime termination means a valid instance existed and must now be cleaned up

## Relationship to the function documents

This lifecycle document must be read together with the individual function notes for:

* `FbxQueryMountMsg()`
* `FbxReturnMountMsg()`
* `FbxSetupFS()`
* `FbxEventLoop()`
* `FbxCleanupFS()`

Those function notes document each entry point individually.

This document describes how they fit together as one lifecycle.

## Notes

This document describes the public lifecycle that is already explicit from:

* public function prototypes
* startup-message handling behavior
* reviewed setup, runtime, and teardown paths

It distinguishes deliberately between:

* startup-message handling
* instance creation
* active runtime
* final teardown


# FbxEventLoop

> Status: working draft
> Sources: public headers, implementation review
> Goal: explicit documentation of the public runtime contract of `FbxEventLoop()`

## Purpose

`FbxEventLoop()` runs the active filesysbox runtime for a previously created filesystem instance.

It is the public entry point for the runtime phase of the filesysbox lifecycle.

This function is called only after successful setup through `FbxSetupFS()` and before final teardown through `FbxCleanupFS()`.

## Synopsis

```
LONG FbxEventLoop(struct FbxFS * fs);
```

## Public role in the lifecycle

`FbxEventLoop()` is the runtime phase of the normal public lifecycle.

The normal order is:

1. `FbxSetupFS()`
2. `FbxEventLoop()`
3. `FbxCleanupFS()`

That means `FbxEventLoop()` is not a setup helper and not a teardown helper. It is the active execution phase of a mounted filesysbox instance.

## Input

`FbxEventLoop()` takes one public input:

* `fs`

`fs` must be a valid filesysbox instance returned by `FbxSetupFS()`.

The practical rule is:

* do not call `FbxEventLoop()` with `NULL`
* do not call `FbxEventLoop()` with an instance that was never successfully created
* do not call `FbxEventLoop()` with an instance that has already been cleaned up

## Result

`FbxEventLoop()` returns a `LONG`.

In the reviewed implementation, the practical observed result is that the function returns when the runtime phase ends.

For lifecycle purposes, the important meaning of the return is:

* runtime is over for this active phase
* teardown must now follow through `FbxCleanupFS()`

The return value should therefore be understood primarily as a runtime-exit boundary, not as a rich public status-reporting interface.

## What runtime means

During `FbxEventLoop()`, filesysbox performs the active work of the mounted filesystem instance.

The reviewed implementation shows runtime activity such as:

* packet handling
* notify reply handling
* signal-driven work
* timer-related work
* volume setup and cleanup transitions
* backend hook invocation through the installed operation table

This means `FbxEventLoop()` is not a passive wait function. It is the main runtime loop of the instance.

## Relationship to setup

`FbxEventLoop()` requires that setup has already completed successfully.

Before runtime begins:

* `FbxSetupFS()` must already have created the instance
* the backend operation table must already have been normalized and installed
* backend-private context must already have been attached
* startup-message handling associated with setup must already have been completed if setup used a startup message

That makes `FbxSetupFS()` the creation boundary and `FbxEventLoop()` the execution boundary.

## Relationship to teardown

`FbxEventLoop()` does not destroy the instance.

Returning from `FbxEventLoop()` does not replace `FbxCleanupFS()`.

The public lifecycle remains:

* setup through `FbxSetupFS()`
* runtime through `FbxEventLoop()`
* teardown through `FbxCleanupFS()`

That means the caller must still perform cleanup after the event loop returns.

## Runtime ownership and lifetime

During `FbxEventLoop()`:

* filesysbox owns the active runtime state of the `struct FbxFS *` instance
* the backend-private context attached through `udata` must remain valid
* installed callback tables remain active through the runtime phase

The caller still retains lifecycle responsibility in this sense:

* once runtime ends, the caller must still call `FbxCleanupFS()`

## Runtime and backend hooks

`FbxEventLoop()` is the phase in which backend hooks actually become active in normal operation.

This means backend-provided hooks in `struct fuse_operations` may be invoked during this phase as filesysbox processes filesystem activity.

A backend author should therefore treat the start of `FbxEventLoop()` as the start of real mounted runtime behavior.

## Runtime and current-volume behavior

The reviewed implementation shows that current-volume setup, cleanup, and transition handling occur during runtime.

That means `FbxEventLoop()` is not limited to simple packet dispatch. It is also where visible runtime state such as current-volume transitions is processed.

This is one reason the runtime phase is distinct from setup: setup creates the instance, but runtime processes live filesystem state.

## Runtime exit

`FbxEventLoop()` returns when the active runtime phase ends.

For public API purposes, the most important fact is the state transition:

* before return, the instance is in active runtime
* after return, the active runtime phase is over
* after return, cleanup is required

The lifecycle meaning of return is therefore more important than the current numeric return value.

## Minimal backend usage

A minimal backend should use `FbxEventLoop()` in the normal lifecycle only after successful setup.

The practical pattern is:

1. prepare backend-private context
2. prepare and pass the operation table to `FbxSetupFS()`
3. if setup succeeds, call `FbxEventLoop()`
4. when `FbxEventLoop()` returns, call `FbxCleanupFS()`

A minimal backend should not try to bypass this runtime phase if it wants a normal mounted filesysbox instance.

## Practical rules for callers and backend authors

A caller or backend author should follow these rules:

* call `FbxEventLoop()` only with a valid instance returned by `FbxSetupFS()`
* treat `FbxEventLoop()` as the active lifetime of the mounted instance
* keep backend-private context valid throughout runtime
* do not assume that returning from `FbxEventLoop()` also performs cleanup
* always follow runtime termination with `FbxCleanupFS()`

## Relationship to other public contracts

`FbxEventLoop()` must be read together with:

* `FbxSetupFS()` for instance creation
* `FbxCleanupFS()` for final teardown
* `struct fuse_operations` for the active backend hook contract during runtime
* the error model, because runtime validation and current-volume state are processed during this phase

## Notes

This document describes the runtime contract already made explicit through:

* the public prototype of `FbxEventLoop()`
* the normal public lifecycle
* reviewed runtime processing behavior

It distinguishes deliberately between:

* instance creation
* active runtime execution
* final teardown


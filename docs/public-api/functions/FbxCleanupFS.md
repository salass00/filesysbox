# FbxCleanupFS

> Status: working draft
> Sources: public headers, implementation review
> Goal: explicit documentation of the public teardown contract of `FbxCleanupFS()`

## Purpose

`FbxCleanupFS()` destroys a previously created filesysbox-managed filesystem instance and releases its internal resources.

It is the public teardown entry point of the filesysbox lifecycle.

This function is the lifecycle counterpart of `FbxSetupFS()`.

## Synopsis

```
void FbxCleanupFS(struct FbxFS * fs);
```

## Public role in the lifecycle

`FbxCleanupFS()` is the teardown phase of the normal public lifecycle.

The normal order is:

1. `FbxSetupFS()`
2. `FbxEventLoop()`
3. `FbxCleanupFS()`

This makes `FbxCleanupFS()` the public destruction boundary of a filesysbox instance.

## Input

`FbxCleanupFS()` takes one public input:

* `fs`

The reviewed behavior shows that `fs` may be:

* a valid filesysbox instance returned by `FbxSetupFS()`
* `NULL`

This means defensive cleanup through `FbxCleanupFS(NULL)` is permitted.

## Result

`FbxCleanupFS()` does not return a result value.

Its public effect is teardown of the supplied instance if one is present.

For lifecycle purposes, the important meaning is simple:

* after cleanup, the instance is gone
* after cleanup, the instance must not be used again

## Relationship to setup

`FbxCleanupFS()` is the public teardown counterpart of `FbxSetupFS()`.

That means:

* `FbxSetupFS()` creates the instance
* `FbxCleanupFS()` destroys it

This pairing defines the lifetime of the public `struct FbxFS *` handle.

## Relationship to runtime

`FbxCleanupFS()` is called after runtime has ended.

Returning from `FbxEventLoop()` does not destroy the instance by itself.

The caller must still call `FbxCleanupFS()` to complete the lifecycle.

That means the public runtime boundary and the public teardown boundary are separate:

* `FbxEventLoop()` ends active runtime
* `FbxCleanupFS()` ends instance lifetime

## Relationship to failed setup

If `FbxSetupFS()` fails, it returns `NULL` and does not produce a usable instance.

The reviewed setup path also shows that filesysbox cleans up partial internal setup state before returning failure.

That means the practical public rule is:

* if setup failed and returned `NULL`, there is no valid instance for the caller to clean up
* if setup succeeded, later cleanup is the caller’s responsibility

This distinguishes failed setup clearly from normal teardown of a real instance.

## Teardown effect

The reviewed implementation shows that `FbxCleanupFS()` performs full instance teardown.

That includes cleanup of runtime state such as:

* message ports
* helper-process references
* signal allocations
* timer-related state
* memory pools
* character-conversion resources
* current instance state

For API documentation purposes, the important fact is:

* `FbxCleanupFS()` is full instance teardown, not a light helper or partial reset

## Ownership and lifetime

### Public instance handle

The `struct FbxFS *` returned by `FbxSetupFS()` remains the public handle of the instance until `FbxCleanupFS()` completes.

After `FbxCleanupFS()` returns for a valid instance:

* that handle must be considered dead
* it must not be reused
* it must not be passed again into runtime or other instance-bound functions as though it were still active

### Backend private context

The backend-private context attached through `udata` belongs to backend-side lifetime management.

Because `udata` is stored, not copied, the backend remains responsible for the lifetime of the backend-owned object itself.

`FbxCleanupFS()` ends filesysbox’s use of the instance and therefore ends the period during which backend-private context must remain valid for that instance.

### `NULL` cleanup

Passing `NULL` to `FbxCleanupFS()` is permitted.

That allows defensive cleanup code without inventing a special caller-side branch for the no-instance case.

## Teardown boundary

`FbxCleanupFS()` is the final lifecycle boundary of the instance.

After it returns:

* runtime is over
* setup state is gone
* the public instance lifetime is over

This is the point at which the caller may consider the filesysbox instance fully finished.

## Minimal backend usage

A minimal backend should use `FbxCleanupFS()` exactly once for each successfully created instance.

The practical minimal pattern is:

1. call `FbxSetupFS()`
2. if setup succeeds, call `FbxEventLoop()`
3. when runtime ends, call `FbxCleanupFS()`

A minimal backend should not treat `FbxCleanupFS()` as optional after successful setup.

## Practical rules for callers and backend authors

A caller or backend author should follow these rules:

* call `FbxCleanupFS()` after successful setup and finished runtime
* do not call `FbxCleanupFS()` as a substitute for checking whether setup succeeded
* treat the instance as dead immediately after cleanup returns
* do not attempt to re-enter runtime after cleanup
* use `FbxCleanupFS(NULL)` only as defensive no-instance cleanup, not as a substitute for lifecycle discipline

## Relationship to other public contracts

`FbxCleanupFS()` must be read together with:

* `FbxSetupFS()` for instance creation
* `FbxEventLoop()` for runtime
* the lifecycle document for the full setup → runtime → teardown sequence
* backend-private lifetime rules tied to `udata`

## Notes

This document describes the teardown contract already made explicit through:

* the public prototype of `FbxCleanupFS()`
* the normal lifecycle structure
* reviewed teardown behavior

It distinguishes deliberately between:

* setup failure that never produced a usable instance
* runtime termination through `FbxEventLoop()`
* final instance destruction through `FbxCleanupFS()`


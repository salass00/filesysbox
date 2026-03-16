# FbxSignalDiskChange

> Status: working draft
> Sources: public headers, implementation review
> Goal: explicit documentation of the public disk-change notification contract of `FbxSignalDiskChange()`

## Purpose

`FbxSignalDiskChange()` notifies filesysbox that the backend or medium state associated with a live instance has changed.

It is part of the optional public integration surface for backends or wrappers that need to tell filesysbox that current-volume or media state should be reconsidered.

## Synopsis

```
void FbxSignalDiskChange(struct FbxFS * fs);
```

## Public role in the lifecycle

`FbxSignalDiskChange()` is not a lifecycle transition.

It does not:

* create an instance
* begin runtime
* destroy an instance

It is a runtime-facing notification function that applies to an already existing filesysbox instance.

## Input

`FbxSignalDiskChange()` takes one public input:

* `fs`

`fs` is the filesysbox instance whose current media or volume state should be reconsidered.

For the practical public contract, it must be a live instance created by `FbxSetupFS()`.

## Result

`FbxSignalDiskChange()` does not return a result value.

Its public effect is to notify filesysbox that current backend or media state has changed and that volume-related runtime state should be updated accordingly.

## Relationship to current-volume state

The reviewed implementation shows that filesysbox has explicit current-volume state handling, including:

* no current volume
* invalid or not-formatted backend layout
* backend setup or resource failure
* valid current volume

`FbxSignalDiskChange()` belongs to this part of the public runtime model.

Its role is not to report metadata about one file or one directory entry. Its role is to notify filesysbox that the wider current-volume or medium state may no longer be the same as before.

## Relationship to runtime

`FbxSignalDiskChange()` belongs to live-instance runtime behavior.

That means:

* it is relevant after successful setup
* it is relevant while the instance is active
* it belongs to runtime state transition handling rather than to setup-time structure definition

It is therefore part of the optional runtime integration surface.

## Relationship to disk-change support

The public setup flag `FBXF_ENABLE_DISK_CHANGE_DETECTION` enables filesysbox disk-change detection support.

`FbxSignalDiskChange()` is related to the same general domain of current-media change handling, but through an explicit public notification call rather than passive setup flags alone.

This makes the function relevant when backend-side or wrapper-side code learns that medium or volume state has changed and wants filesysbox to react.

## Relationship to locks, handles, and current context

The reviewed implementation shows that current-volume changes matter because many runtime operations depend on current-volume state.

That means disk or media change is not isolated from the rest of the API. It can affect:

* future path resolution
* lock validity in practice
* handle validity in practice
* visible current-volume behavior

For public API purposes, the important contract statement is:

* `FbxSignalDiskChange()` notifies filesysbox about a change in the wider mounted state, not merely about one object-level event

## Relationship to callback and integration functions

`FbxSignalDiskChange()` belongs to the same optional integration surface as:

* `FbxSetSignalCallback()`
* `FbxInstallTimerCallback()`
* `FbxUninstallTimerCallback()`

All of these functions operate on a live filesysbox instance and extend the public integration surface beyond the minimal required lifecycle.

## Instance requirements

`FbxSignalDiskChange()` operates on an existing filesysbox instance.

The practical rule is:

* call it only on an instance created by `FbxSetupFS()`
* treat it as a runtime notification function
* do not treat it as a startup or teardown function

## Minimal backend relevance

A minimal backend does not need `FbxSignalDiskChange()` in order to mount and run normally.

This function belongs to optional runtime integration rather than to the minimal required mounted path.

Its importance is therefore:

* API completeness
* media-aware backends
* wrappers or integrations that need explicit current-state change notification

## Practical rules for callers and backend authors

A caller or backend author should follow these rules:

* call `FbxSignalDiskChange()` only on a live filesysbox instance
* use it only when backend or medium state has changed in a way that affects current-volume handling
* treat it as a whole-instance state-change notification, not as a file-level metadata helper
* do not use it as a substitute for setup, runtime entry, or teardown

## Relationship to other public contracts

`FbxSignalDiskChange()` must be read together with:

* `FbxSetupFS()` because it requires an existing instance
* `FbxEventLoop()` because it belongs to live runtime behavior
* `FbxCleanupFS()` because no integration function outlives the instance
* the error model because current-volume state directly affects visible failure behavior
* the setup flag `FBXF_ENABLE_DISK_CHANGE_DETECTION` because both concern disk or medium change handling

## Notes

This document describes the disk-change notification contract already made explicit through:

* the public prototype of `FbxSignalDiskChange()`
* the reviewed role of current-volume state in runtime behavior
* the existence of explicit filesysbox disk-change support

It distinguishes deliberately between:

* lifecycle functions
* object-level metadata operations
* whole-instance media or current-volume state change notification


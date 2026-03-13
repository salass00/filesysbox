# Error model

> Status: working draft  
> Sources: public headers, implementation review  
> Goal: explicit documentation of the practical error model visible in the public filesysbox API and in the reviewed implementation paths

## Purpose

This document describes the practical error model of filesysbox as it is visible through the public API and through the reviewed implementation paths.

The purpose of this document is to state explicitly:

- how setup failure differs from runtime failure
- how backend hook failure differs from filesysbox-side rejection
- how current-volume state affects visible failure behavior
- how callers and backend authors should think about public error propagation

This document does not attempt to define a complete hook-by-hook backend error table. It documents the practical error model that is already visible from the reviewed public and implementation-facing paths.

## General structure of the error model

filesysbox sits between:

- a backend that provides FUSE-style hooks
- AmigaOS-facing filesystem behavior
- filesysbox-internal state and helper logic

Because of that position, error handling is layered.

In practice, failures may originate from:

- setup or initialization failure
- backend hook failure
- filesysbox-side validation before a backend hook is reached
- current-volume state
- write-protection or read-only state
- invalid locks, stale handles, or cross-volume mismatch
- startup-message reply handling

The practical error model therefore cannot be reduced to backend hook return values alone.

## Setup failure

`FbxSetupFS()` has the clearest public failure contract.

The public function returns:

- a valid `struct FbxFS *` on success
- `NULL` on failure

This is the public setup failure boundary.

### Practical meaning

If `FbxSetupFS()` returns `NULL`:

- setup did not complete successfully
- no usable filesysbox instance exists for the caller
- the caller must not proceed to `FbxEventLoop()` with that result

If `FbxSetupFS()` succeeds:

- a usable filesysbox instance exists
- the caller is responsible for later calling `FbxCleanupFS()`

### Setup cleanup behavior

The reviewed setup path shows that filesysbox cleans up partially initialized setup state before returning failure.

That means setup failure is handled as:

- fail the setup
- tear down partial internal state
- return `NULL`

This is an important part of the public lifecycle contract.

## Startup-message failure versus normal runtime failure

The startup-message path is separate from normal runtime operation.

The relevant public functions are:

- `FbxQueryMountMsg()`
- `FbxReturnMountMsg()`
- `FbxSetupFS()`

### Practical division of roles

- `FbxQueryMountMsg()` inspects startup context
- `FbxReturnMountMsg()` explicitly replies to the startup message
- `FbxSetupFS()` may consume the startup path and perform the reply itself

This means startup reply handling is not just another runtime error path. It belongs to setup control flow.

When `FbxSetupFS()` is used with a valid startup message, filesysbox handles the reply path itself on both success and failure.

## Runtime failure

Once setup has succeeded and the instance is running, failure becomes distributed.

At runtime, filesysbox processes:

- packet dispatch
- metadata translation
- lock validation
- volume-state checks
- backend hook invocation
- notify handling
- helper-process state

As a result, a failed user-visible operation may come from different layers even when the backend itself is not the only failing component.

The practical runtime error model is therefore built from several categories.

## Backend hook failure

Backend hooks are one source of failure.

When a backend hook fails, that failure participates in the filesysbox error model.

However, the practical public error model is not just “backend returned an error”.

The reviewed code shows that filesysbox also performs its own checks and maintains its own error state, especially through `fs->r2`.

That means backend failure is one input into the visible error model, not the whole model by itself.

This document therefore treats backend hook failure as one category inside the larger filesysbox error model.

## filesysbox-side error state

The reviewed implementation shows that filesysbox carries explicit AmigaOS-facing error state through `fs->r2`.

That is important because it means:

- a backend hook result is not the same thing as the final visible error state
- filesysbox may set a visible DOS-style error without relying solely on the backend
- filesysbox may reject an operation before the backend hook is even reached

This is one of the defining features of the practical filesysbox error model.

## Current-volume state

Current-volume state is a major part of visible failure behavior.

filesysbox distinguishes multiple `currvol` states.

The reviewed code documents these practical categories:

- `NULL` means no current volume
- `(APTR)-1` means invalid or not-formatted backend layout
- `(APTR)-2` means backend setup or resource failure
- any other value below `(APTR)-2` is treated as a valid `struct FbxVolume *`

These states are not abstract internals only. They are used directly in runtime checks and influence visible error outcomes.

## `CHECKVOLUME()` behavior

The `CHECKVOLUME()` macro shows how current-volume state maps into visible runtime failure.

### No current volume

When `fs->currvol` is `NULL`:

- if `fs->inhibit` is set, filesysbox sets `fs->r2 = ERROR_NOT_A_DOS_DISK`
- otherwise, filesysbox sets `fs->r2 = ERROR_NO_DISK`

This means the public-facing “no current volume” case is already split into two practical outcomes:

- inhibited access behaves as “not a DOS disk”
- normal absence of media behaves as “no disk”

### Bad volume state

When `fs->currvol` is one of the bad sentinel values:

- filesysbox sets `fs->r2 = ERROR_NOT_A_DOS_DISK`

This means invalid layout and backend setup/resource failure are both mapped into practical “not a DOS disk” behavior in the reviewed runtime checks.

## Write-protection and read-only rejection

filesysbox also rejects write attempts independently of backend hook failure when the current volume is not writable.

The `CHECKWRITABLE()` macro shows that filesysbox sets:

- `fs->r2 = ERROR_DISK_WRITE_PROTECTED`

when either of these is true:

- `fs->currvol->writeprotect` is set
- `fs->currvol->vflags & FBXVF_READ_ONLY` is set

This is important because it shows a distinct class of filesysbox-side rejection:

- the backend does not need to be called
- filesysbox can reject the operation directly based on current volume state

## Invalid lock and stale-context rejection

filesysbox also performs explicit lock validation.

The `CHECKLOCK()` macro sets:

- `fs->r2 = ERROR_INVALID_LOCK`

when `FbxCheckLock()` fails.

This means lock validity is part of the visible error model and is not delegated wholly to the backend.

The reviewed code also shows repeated checks of the form:

- `lock->fsvol != fs->currvol`

in file and directory operation paths.

That means a lock can become invalid for practical operation purposes even if it was once valid, because the current volume context has changed.

This is another filesysbox-side rejection category independent of pure backend hook failure.

## Cross-volume and stale-handle effects

The reviewed file and directory paths repeatedly compare the volume associated with a lock or handle against the current volume.

This means filesysbox treats some operations as invalid when:

- the lock belongs to a different volume than the current one
- the current volume has changed since the lock was created
- the filesystem state visible to the caller is no longer the same logical volume context

This is not just a backend policy question. It is part of filesysbox-side state validation.

## Metadata and examine-path errors

In metadata and examine-style paths, filesysbox translates backend metadata into AmigaOS-facing structures.

Failure in these paths may come from:

- backend inability to provide metadata
- filesysbox-side inability to interpret current volume state
- invalid object or lock context
- missing object state

This means metadata-facing failure belongs to the same layered model:

- backend hook failure may initiate it
- filesysbox-side validation and conversion determine the visible outcome

## Volume setup failure after backend initialization

Volume setup is a distinct practical failure stage.

The reviewed `volume.c` path shows that filesysbox may fail while trying to establish the current volume even after backend initialization has already progressed.

Examples include:

- backend failure while obtaining root metadata
- failure allocating `FbxVolume`
- failure adding the volume to the DOS list
- invalid or non-DOS backend layout

These do not all collapse into the same practical state.

Internally they are represented through distinct sentinel values and later checked through volume-state logic.

## Media-change and runtime state transition effects

The reviewed implementation shows that current-volume state is not a once-for-all setup result.

It can change during runtime.

That means some visible failures arise not because the backend hook itself changed behavior, but because:

- media state changed
- current volume disappeared
- current volume became invalid
- lock and handle state no longer matches current volume state

This is a defining part of the filesysbox runtime error model.

## Query and helper functions

Not all public functions participate equally in the error model.

### Pure helpers or small query functions

Functions such as:

- `FbxVersion()`
- `FbxFuseVersion()`

do not participate in the same runtime error model as filesystem operations.

They are essentially side-effect-free public queries.

### Output-buffer helpers

Functions such as:

- `FbxGetSysTime()`
- `FbxGetUpTime()`
- `FbxCopyStringBSTRToC()`
- `FbxCopyStringCToBSTR()`

operate through caller-supplied output storage and do not form part of the layered filesystem operation error model described above.

### Setup- and runtime-integrated functions

Functions such as:

- `FbxSetupFS()`
- `FbxEventLoop()`
- `FbxCleanupFS()`
- `FbxQueryFS()`
- callback-related configuration functions

belong to lifecycle or integration behavior and must be interpreted through their specific contract rather than through lock/file operation error rules.

## Practical classification of error sources

The practical filesysbox error model is best understood in these categories:

### 1. Setup failure

Public boundary:

- `FbxSetupFS()` returns `NULL`

### 2. Startup-message control failure

Public boundary:

- startup reply handled through `FbxReturnMountMsg()` or by `FbxSetupFS()`

### 3. Current-volume failure

Visible through:

- `ERROR_NO_DISK`
- `ERROR_NOT_A_DOS_DISK`

depending on current-volume state and inhibition state

### 4. Writability failure

Visible through:

- `ERROR_DISK_WRITE_PROTECTED`

### 5. Lock or context failure

Visible through:

- `ERROR_INVALID_LOCK`

and through practical stale-volume mismatch checks

### 6. Backend hook failure

Participates as one part of the error model, but not the whole model by itself

## Practical rules for backend authors

A backend author should understand the public error model this way:

- a failing backend hook is only one layer of visible failure
- filesysbox can reject operations before backend hook invocation
- volume state and lock state are first-class parts of visible failure behavior
- enabling or changing current-volume state changes user-visible failure outcomes even when the backend hook set itself remains unchanged

This is especially important when reasoning about:

- stale locks
- media changes
- read-only behavior
- not-a-DOS-disk behavior
- setup failure versus runtime failure

## Relationship to other public contracts

The error model must be read together with:

- `FbxSetupFS()`, `FbxEventLoop()`, and `FbxCleanupFS()`
- the backend operation contract in `struct fuse_operations`
- the metadata contract in `struct fbx_stat`
- the handle contract in `struct fuse_file_info`
- current-volume state as used by filesysbox runtime validation

The practical error model is the result of these contracts interacting, not a separate isolated subsystem.

## Notes

This document describes the practical error model already visible in:

- public setup behavior
- reviewed runtime checks
- current-volume state handling
- lock validation
- writability validation

It deliberately treats the error model as layered:

- setup and startup control
- filesysbox-side validation
- current-volume state
- backend hook failure

That layered view matches the reviewed code paths more accurately than a single flat list of backend error returns.

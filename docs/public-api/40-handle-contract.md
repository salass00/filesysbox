# Handle contract

> Status: working draft  
> Sources: public headers, implementation review  
> Goal: explicit documentation of the public handle contract represented by `struct fuse_file_info`

## Purpose

This document describes the public handle contract between a filesysbox backend and filesysbox.

The central structure for this contract is `struct fuse_file_info`.

This structure carries per-open or per-handle state through file and directory operations such as:

- `open`
- `read`
- `write`
- `flush`
- `release`
- `opendir`
- `readdir`
- `releasedir`
- `fgetattr`
- `ftruncate`

The purpose of this document is to state explicitly:

- which fields are controlled by filesysbox
- which fields are intended for backend use
- how file and directory handle state is carried across calls
- which fields are currently compatibility surface rather than part of the active practical contract

## Public structure overview

`struct fuse_file_info` contains these public fields:

- `flags`
- `fh_old`
- `writepage`
- `direct_io`
- `keep_cache`
- `flush`
- `nonseekable`
- `padding`
- `fh`
- `lock_owner`

The public header comment already gives a strong summary of how filesysbox treats this structure:

- filesysbox sets `flags`
- filesysbox reads `nonseekable`
- the rest is currently cleared and otherwise untouched
- `fh_old` and `fh` are safe to be used by the filesystem or backend

This comment is the core of the currently documentable practical handle contract.

## General contract

`struct fuse_file_info` is the per-handle state carrier used by filesysbox when a backend works with opened files or directories.

The practical division of responsibility is:

- filesysbox provides open-context information through `flags`
- the backend may place its own per-handle state in `fh` or `fh_old`
- the backend may declare a handle non-seekable through `nonseekable`
- the remaining fields are currently not part of the active practical backend contract in the reviewed paths

This makes `struct fuse_file_info` a mixed contract object:

- part filesysbox-controlled input
- part backend-controlled per-handle state
- part compatibility surface

## `flags`

`flags` is a filesysbox-provided input field.

The public header explicitly states that filesysbox sets it.

For backend authors, that means:

- `flags` describes the open-time context
- `flags` is not backend-owned persistent state
- a backend may inspect it in `open()` and related handle-creation paths

The current handle contract therefore treats `flags` as input, not storage.

## `fh`

`fh` is the primary backend-owned handle field.

The public header explicitly states that `fh` is safe to be used by the filesystem or backend.

In the practical handle contract:

- the backend may store opaque per-handle state in `fh`
- filesysbox carries that state through subsequent handle-aware calls
- the backend remains responsible for the meaning and lifetime of whatever `fh` represents

For new backends, `fh` should be treated as the preferred handle field.

Typical contents of `fh` include:

- an internal file object pointer
- a backend-specific numeric handle
- a directory enumeration context
- another opaque backend-owned per-open object

## `fh_old`

`fh_old` is also safe for backend use.

The public header states that both `fh_old` and `fh` are safe to be used by the filesystem or backend.

That means `fh_old` is not forbidden or dead.

However, for new backends the practical recommendation is:

- prefer `fh`
- treat `fh_old` as compatibility surface that remains available when needed

This gives the current handle contract a clear primary field without inventing a prohibition that the public header does not state.

## `nonseekable`

`nonseekable` is a backend-facing output field.

The public header explicitly states that filesysbox reads it.

That means:

- the backend may set `nonseekable`
- filesysbox may later use that information when handling the open object

This field is therefore part of the active practical contract.

For normal regular files, a backend may leave it clear.

For streams or otherwise non-seekable objects, the backend should set it appropriately.

## `writepage`, `direct_io`, `keep_cache`, `flush`, `padding`, `lock_owner`

In the reviewed practical contract, these fields are not part of the active documented handle core.

The public header comment states that fields other than `flags`, `nonseekable`, `fh_old`, and `fh` are currently cleared and otherwise untouched.

For current API documentation, that means:

- they are part of the public structure
- they should not be documented as active required backend knobs in the current practical contract
- a backend should not assume additional semantic handling for them unless documented elsewhere

This is not a statement that these fields are invalid. It is a statement about the currently documented practical contract.

## File handles and directory handles

The same structure is used for both file handles and directory handles.

This gives the handle contract two common patterns.

### File-handle pattern

Typical file-handle flow:

- `open()` validates the object and stores backend handle state in `fh`
- `read()`, `write()`, `flush()`, `fgetattr()`, `ftruncate()`, or similar hooks use that handle state
- `release()` ends the lifetime of that handle state

### Directory-handle pattern

Typical directory-handle flow:

- `opendir()` validates the object and stores backend directory state in `fh`
- `readdir()` uses that state
- `releasedir()` ends the lifetime of that directory state

The structure does not distinguish these cases by separate storage fields. The meaning of the stored handle state is determined by the hook path that created it.

## Lifetime and ownership

The ownership model is straightforward.

### Filesysbox-owned parts

Filesysbox owns the meaning of:

- `flags` as open-context input

Filesysbox also currently reads:

- `nonseekable`

### Backend-owned parts

The backend owns the meaning and lifetime of any state stored in:

- `fh`
- `fh_old`

That means:

- filesysbox carries the stored state
- filesysbox does not become owner of the backend-private object represented there
- creation and destruction of that backend-private handle state remain backend responsibilities

### Practical lifetime rule

The practical lifetime rule is:

- create backend-private handle state in `open()` or `opendir()`
- use it in later handle-aware hooks
- destroy it in `release()` or `releasedir()`

That is the currently documentable handle-lifetime contract.

## Minimal backend contract

For a minimal readonly backend, the practical handle contract is very small.

A minimal backend only needs to rely on:

- `flags` as filesysbox-provided open context
- `fh` as backend-private per-handle storage
- optional `nonseekable` when the opened object is not seekable

No other `struct fuse_file_info` fields are part of the minimal practical contract in the currently reviewed paths.

## Recommended backend practice

A backend should:

- inspect `flags` in `open()`
- store backend-private per-handle state in `fh`
- use `fh` consistently in handle-aware hooks
- free or invalidate backend-private handle state in `release()` or `releasedir()`
- set `nonseekable` only when the opened object is genuinely non-seekable

For new backends:

- prefer `fh` over `fh_old`
- treat the remaining fields as compatibility surface, not as part of the active practical contract

## Relationship to other public contracts

The handle contract must be read together with:

- `struct fuse_operations`, which defines which hooks receive `struct fuse_file_info`
- `FbxSetupFS()`, which installs the operation table and makes handle-aware hooks active
- `struct fbx_stat`, because metadata may also be requested for open handles through `fgetattr`

The handle contract is especially important for:

- `open`, `read`, `write`, `flush`, `release`
- `opendir`, `readdir`, `releasedir`
- `fgetattr`
- `ftruncate`

## Notes

This document describes the currently explicit practical contract already supported by:

- the public header comment for `struct fuse_file_info`
- the reviewed operation flow expected by filesysbox

It distinguishes deliberately between:

- active handle fields in the current practical contract
- backend-owned storage fields
- remaining structure fields that currently belong to compatibility surface rather than to the documented handle core

# Backend operation table

> Status: working draft  
> Sources: public headers, implementation review  
> Goal: explicit documentation of the public backend operation contract exposed through `struct fuse_operations`

## Purpose

This document describes the public backend operation table used by filesysbox.

The backend operation table is the structure through which a filesystem backend provides filesystem operations to filesysbox. It follows a FUSE-style callback model and is passed to `FbxSetupFS()` together with `opssize`.

This document states explicitly:

- what `struct fuse_operations` is used for
- how `opssize` affects the contract
- which documented setup flags alter backend expectations
- which fallback behavior is already documented
- which hook groups form the practical minimal readonly subset

## Public structure overview

`struct fuse_operations` contains callback slots for:

- metadata lookup
- path-based object operations
- file-handle operations
- directory-handle operations
- setup and teardown hooks
- optional extended or advanced operations

Each hook has a fixed public function type in the header.

Most operation hooks return `int`.

The lifecycle hooks differ:

- `init` returns `void *`
- `destroy` returns `void`

This is the public callback shape. The public contract begins with the structure itself and with the callback types defined in the header.

## General contract

A backend provides a `struct fuse_operations` instance and passes it to `FbxSetupFS()`.

`FbxSetupFS()` copies the supplied table internally.

The copy is limited by `opssize`.

This gives the operation table two roles at once:

- it is the public backend callback contract
- it is a compatibility boundary controlled through `opssize`

## `opssize` semantics

`opssize` defines how much of the supplied `struct fuse_operations` is valid.

The public `FbxSetupFS` documentation states that the supplied operations table is passed together with its size, and implementation review confirms that filesysbox copies only that valid prefix.

The practical rule is:

- fields beyond `opssize` are treated as absent
- only the declared valid prefix participates in setup-time normalization
- a backend may intentionally provide a smaller valid subset rather than the full structure size

This matters for both compatibility and minimal backends.

## Public filesysbox setup flags

The public setup tag `FBXT_FSFLAGS` controls filesysbox-specific flags that affect backend expectations.

The currently documented public flags are:

- `FBXF_ENABLE_UTF8_NAMES`
- `FBXF_ENABLE_DISK_CHANGE_DETECTION`
- `FBXF_USE_INO`
- `FBXF_USE_FILL_DIR_STAT`

These flags do not redefine the callback table itself, but they alter the practical backend contract around naming, object identity, directory metadata, and disk-change behavior.

## Meaning of the public filesysbox flags

### `FBXF_ENABLE_UTF8_NAMES`

This flag declares that the filesystem uses UTF-8 encoded filenames.

When enabled, filesysbox opens the resources needed for character conversion.

A backend that uses UTF-8 names should enable this flag.

A backend that does not use UTF-8 names should not enable it.

### `FBXF_ENABLE_DISK_CHANGE_DETECTION`

This flag enables disk-change detection through `TD_ADDCHANGEINT`.

The `FbxSetupFS` documentation explicitly limits this to trackdisk-device-based filesystems and notes that a startup message is required for this use.

This is therefore not a general backend feature. It is a specific integration feature for relevant media-backed filesystems.

### `FBXF_USE_INO`

This flag declares that the backend provides meaningful `st_ino` values.

When it is enabled, filesysbox uses backend-provided object identity instead of generating path-based identity.

This flag therefore connects the backend operation contract directly to the metadata contract.

### `FBXF_USE_FILL_DIR_STAT`

This flag declares that valid stat data is passed to the `readdir()` callback.

This is not merely advisory. It is a backend assertion about directory-entry metadata behavior.

A backend should enable it only when `readdir()` really supplies valid `struct fbx_stat` data for returned entries.

## Setup-time normalization and documented fallbacks

`FbxSetupFS()` does not just store the callback table. It also normalizes it.

The documented and observed fallback behavior includes:

- `opendir` falls back to `open`
- `releasedir` falls back to `release`
- `fgetattr` falls back to `getattr`
- `ftruncate` falls back to `truncate`

In addition, missing hooks may be replaced by internal dummy handlers.

This means the callback contract is not “all-or-nothing”. Some missing hooks are tolerated because filesysbox already documents or implements compatible fallback behavior.

## Hook classes

### Metadata hooks

These hooks provide object metadata:

- `getattr`
- `fgetattr`

`getattr` is the primary path-based metadata hook.

`fgetattr` is the handle-aware metadata hook.

If `fgetattr` is absent, filesysbox can fall back to `getattr`.

### File-handle hooks

These hooks operate on opened files:

- `open`
- `read`
- `write`
- `flush`
- `release`
- `fsync`
- `ftruncate`
- `lock`

A minimal readonly backend only needs a small subset of these.

### Directory-handle hooks

These hooks operate on opened directories:

- `opendir`
- `readdir`
- `releasedir`
- `fsyncdir`

If `opendir` and `releasedir` are absent, filesysbox can fall back to `open` and `release`.

### Setup and teardown hooks

These hooks belong to backend lifecycle integration:

- `init`
- `destroy`

They are not mandatory for the smallest practical backend, but they are highly useful for clear backend structure and for setup-time use of `struct fuse_conn_info`.

### Modification hooks

These hooks are for writable or mutating filesystems:

- `mknod`
- `mkdir`
- `unlink`
- `rmdir`
- `symlink`
- `rename`
- `link`
- `chmod`
- `chown`
- `truncate`
- `utime`
- `create`
- `utimens`
- `format`
- `relabel`

A readonly backend does not need them.

### Extended hooks

These hooks cover more advanced behavior:

- `setxattr`
- `getxattr`
- `listxattr`
- `removexattr`
- `access`
- `bmap`

These are optional and are not part of the minimal readonly contract.

## Practical minimal readonly hook set

The public API does not define a separate formally blessed “minimal supported hook set”.

What can be documented is the practical minimal readonly subset that follows from:

- the public structure
- the documented fallback behavior
- the fact that the backend must still provide usable metadata, file access, directory enumeration, and filesystem information

That practical minimal readonly set is:

- `getattr`
- `open`
- `read`
- `release`
- `readdir`
- `statfs`

This is the smallest practical subset for a backend that wants to expose readable files and directories in a useful way.

## Recommended additional hooks for a clear backend

Even for a minimal readonly backend, these additional hooks are strongly recommended:

- `opendir`
- `releasedir`
- `init`
- `destroy`

These are not required to make the interface exist, but they make the backend clearer:

- `opendir` and `releasedir` make directory-handle logic explicit instead of implicit
- `init` and `destroy` make setup-time and teardown-time backend behavior explicit

## Hooks that a first readonly backend does not need

A first readonly backend does not need to implement:

- `write`
- `create`
- `mknod`
- `mkdir`
- `unlink`
- `rmdir`
- `rename`
- `truncate`
- `ftruncate`
- `utime`
- `utimens`
- xattr hooks
- link or symlink hooks
- `lock`
- `format`
- `relabel`
- `bmap`

These belong to writable, advanced, or specialized backends.

## Hook return contract

The public callback table defines the callback return types.

For most hooks, that public return type is `int`.

This document records the callback interface and its structure.

It does not attempt to define the complete error-translation model from backend hook return values to AmigaOS-facing errors.

That error model belongs in the dedicated error-model documentation.

This is a deliberate scope boundary, not an unresolved question.

## Relationship to metadata and handle contracts

The operation table cannot be read in isolation.

It must be read together with:

- the metadata contract represented by `struct fbx_stat`
- the handle contract represented by `struct fuse_file_info`
- the setup contract described by `FbxSetupFS()`

This is especially important for:

- `getattr` and `fgetattr`, which depend on the metadata contract
- `open`, `read`, `release`, `opendir`, `readdir`, and `releasedir`, which depend on the handle contract
- `readdir`, which is further affected by `FBXF_USE_FILL_DIR_STAT`

## Practical rules for backend authors

A backend author should treat this structure as follows:

- provide only the hooks that are truly implemented
- set `opssize` correctly
- rely only on documented or clearly observed fallback behavior
- enable filesysbox-specific setup flags only when the backend really satisfies the stronger associated contract
- keep readonly and writable hook sets clearly separated

## Notes

This document describes the operation-table contract that is already explicit in the public header and in the documented setup behavior of `FbxSetupFS()`.

It distinguishes deliberately between:

- the public callback structure
- setup-time normalization and fallback behavior
- the practical minimal readonly subset
- optional or advanced hooks

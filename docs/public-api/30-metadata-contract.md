# Metadata contract

> Status: working draft  
> Sources: public headers, implementation review  
> Goal: explicit documentation of the public metadata contract between a filesysbox backend and filesysbox

## Purpose

This document describes the public metadata contract represented by `struct fbx_stat`.

A backend supplies `struct fbx_stat` data primarily through:

- `getattr`
- `fgetattr`
- directory-enumeration paths that optionally provide stat data together with names

The purpose of this document is to state explicitly:

- which fields are practically required
- which fields become contractually relevant when specific setup flags are enabled
- which fields are used when available
- which fields currently have no documented reader role in the reviewed metadata-conversion paths

## Public structure overview

`struct fbx_stat` is the public metadata structure used by filesysbox to describe filesystem objects.

It contains fields for:

- object type and mode bits
- inode-like identity
- ownership
- timestamps
- size
- block-related metadata
- additional compatibility-oriented metadata

The structure supports both compatibility-style timestamp names and `struct timespec` timestamp fields.

## General contract

A backend is responsible for filling `struct fbx_stat` consistently enough for filesysbox to present correct filesystem behavior and correct AmigaOS-facing metadata.

The practical metadata contract centers on:

- object type
- size
- timestamps
- object identity when explicitly enabled
- additional metadata when available

Not every field in `struct fbx_stat` has the same practical weight.

## Related public setup flags

Two public setup flags affect the metadata contract directly:

- `FBXF_USE_INO`
- `FBXF_USE_FILL_DIR_STAT`

These flags are passed through `FBXT_FSFLAGS` during `FbxSetupFS()`.

They do not merely decorate the API. They alter how filesysbox treats object identity and directory-entry metadata.

## Required fields in practice

For a minimal useful backend, these fields are the practical core:

### Required for all visible objects

- `st_mode`
- `st_mtim`
- `st_ctim`

### Required for regular files

- `st_size`

These fields are sufficient to support the currently reviewed paths that translate backend metadata into AmigaOS-visible metadata and object behavior.

## `st_mode`

`st_mode` is one of the most important fields in `struct fbx_stat`.

It is used to carry:

- the object type
- relevant protection or mode bits

For a minimal backend, the most important object types are:

- regular file
- directory
- symbolic link, if supported

In the reviewed metadata-conversion paths, filesysbox derives AmigaDOS-facing type and protection information from `st_mode`.

That makes `st_mode` part of the practical required core.

## `st_size`

`st_size` is required for regular files.

In the reviewed paths, filesysbox uses file size information when translating backend metadata into AmigaOS-facing output and file-size-related behavior.

For directories, `st_size` is not part of the currently documented practical core.

The safe rule is:

- provide meaningful `st_size` for regular files
- do not treat directory `st_size` as part of the minimal required subset

## Timestamp fields

### Primary timestamp view

For new backends, the `timespec` view should be treated as the primary semantic representation:

- `st_atim`
- `st_mtim`
- `st_ctim`

The compatibility-style names:

- `st_atime` / `st_atimensec`
- `st_mtime` / `st_mtimensec`
- `st_ctime` / `st_ctimensec`

should be treated as compatibility surface.

### Practically required timestamps

In the reviewed paths, filesysbox actively uses:

- `st_mtim`
- `st_ctim`

`st_ctim` is used when setting the volume date during volume setup.

`st_mtim` is used in examine- and ExAll-style metadata conversion.

That makes these two fields part of the practical required core.

### `st_atim`

In the currently reviewed metadata-conversion paths, `st_atim` does not have a documented reader role.

That means:

- `st_atim` is valid metadata
- but it is not part of the minimal practical contract currently required by the reviewed filesysbox paths

## Object identity and `st_ino`

`st_ino` is conditionally important.

Its contract role becomes explicit when the public setup flag

- `FBXF_USE_INO`

is enabled.

### When `FBXF_USE_INO` is enabled

When this flag is enabled, the backend is declaring that it supplies meaningful inode-like identity through `st_ino`.

In this configuration:

- `st_ino` becomes part of the active metadata contract
- the backend should provide meaningful and stable `st_ino` values

### When `FBXF_USE_INO` is not enabled

When this flag is not enabled, filesysbox can fall back to path-based identity handling.

That means `st_ino` is not part of the same practical requirement in that configuration.

### Practical rule

- if a backend enables `FBXF_USE_INO`, it should provide stable and meaningful `st_ino`
- if a backend does not enable `FBXF_USE_INO`, `st_ino` is not part of the same explicit contract

## Directory-entry stat data

The public setup flag

- `FBXF_USE_FILL_DIR_STAT`

declares that valid stat data is passed together with directory entries in `readdir()`.

This directly connects directory enumeration to the metadata contract.

### Practical rule

When this flag is enabled, the backend is asserting that:

- `readdir()` can supply valid `struct fbx_stat` data for returned entries

Backends should therefore enable this flag only when directory enumeration genuinely provides correct metadata for those entries.

If a backend does not provide valid metadata in `readdir()`, it should not enable this flag.

## Fields used when available

The following fields are used in the currently reviewed metadata-conversion paths, but they are not part of the smallest practical metadata core:

- `st_blocks`
- `st_uid`
- `st_gid`

### `st_blocks`

`st_blocks` contributes block-count information.

If it is not provided meaningfully, filesysbox can derive a block count from file size and block size in at least some reviewed paths.

This makes `st_blocks` useful, but not part of the first required subset.

### `st_uid` and `st_gid`

`st_uid` and `st_gid` contribute owner-related metadata in reviewed examine-style output paths.

They are therefore real metadata inputs, not dead fields, but they are not part of the smallest practical contract needed for a minimal useful backend.

## Fields with no documented reader role in the reviewed metadata-conversion paths

In the currently reviewed metadata-conversion paths, the following fields have no documented reader role:

- `st_blksize`
- `st_nlink`
- `st_rdev`

This statement is intentionally narrow.

It does **not** mean these fields are invalid or forbidden. It means:

- no reader role for these fields has been established in the reviewed metadata-conversion paths used for this document

For the purposes of current API documentation, they should not be presented as part of the practical minimal metadata core.

## Minimal backend contract

For a minimal backend, the practical metadata contract is:

### Always provide

- `st_mode`
- `st_mtim`
- `st_ctim`

### Provide for regular files

- `st_size`

### Provide when explicitly declared by setup flags

- `st_ino` when `FBXF_USE_INO` is enabled

### Provide when available for richer metadata output

- `st_blocks`
- `st_uid`
- `st_gid`

## Recommended backend practice

A backend should:

- always provide meaningful `st_mode`
- always provide meaningful `st_mtim`
- always provide meaningful `st_ctim`
- provide meaningful `st_size` for regular files
- treat `st_atim`, `st_mtim`, and `st_ctim` as the primary timestamp representation
- treat the compatibility timestamp names as compatibility surface, not the preferred internal model

In addition:

- enable `FBXF_USE_INO` only when meaningful `st_ino` values are actually provided
- enable `FBXF_USE_FILL_DIR_STAT` only when `readdir()` supplies valid stat data for directory entries

## Relationship to other public contracts

The metadata contract must be read together with:

- `struct fuse_operations`, especially `getattr`, `fgetattr`, and `readdir`
- `FbxSetupFS()`, because setup flags influence metadata semantics
- `struct fuse_file_info`, because metadata may also be requested for open handles through `fgetattr`

A backend is not complete simply because it implements `getattr()`. It must also provide metadata that remains consistent with:

- directory enumeration
- handle-aware metadata access
- object identity behavior
- setup flags that declare stronger metadata guarantees

## Notes

This document describes the metadata contract that is already explicit from public headers and the reviewed metadata-conversion paths.

It distinguishes deliberately between:

- the practical required core
- conditionally required metadata
- metadata used when available
- fields for which no reader role is currently documented in the reviewed paths

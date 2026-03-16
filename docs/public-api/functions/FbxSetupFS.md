# FbxSetupFS

> Status: working draft
> Sources: public headers, implementation review
> Goal: explicit documentation of the public setup contract of `FbxSetupFS()`

## Purpose

`FbxSetupFS()` creates and initializes a filesysbox-managed filesystem instance.

It is the public setup entry point that binds:

* startup context
* setup tags
* a backend operation table
* backend private context

into one filesysbox runtime instance.

This function is the boundary between:

* pre-setup control flow
* instance creation
* later runtime through `FbxEventLoop()`

## Synopsis

```
struct FbxFS * FbxSetupFS(
struct Message * msg,
const struct TagItem * tags,
const struct fuse_operations * ops,
LONG opssize,
APTR udata
);
```

## Public role in the lifecycle

`FbxSetupFS()` is the public entry point that creates a filesysbox instance.

In the normal lifecycle, the call order is:

1. optionally inspect startup context with `FbxQueryMountMsg()`
2. call `FbxSetupFS()`
3. if setup succeeds, call `FbxEventLoop()`
4. when runtime ends, call `FbxCleanupFS()`

This makes `FbxSetupFS()` the creation phase of the public lifecycle.

## Inputs

`FbxSetupFS()` takes these public inputs:

* `msg`
* `tags`
* `ops`
* `opssize`
* `udata`

Each of them has a distinct role in setup.

### `msg`

`msg` is a startup or mount message, or `NULL`.

When a valid startup message is supplied, setup incorporates that startup context into instance creation.

The reviewed setup behavior also shows that filesysbox handles the reply path for that message itself when setup is used in the normal startup flow.

### `tags`

`tags` is a setup tag list.

The currently documented public setup tags are:

* `FBXT_FSFLAGS`
* `FBXT_FSSM`
* `FBXT_DOSTYPE`
* `FBXT_GET_CONTEXT`
* `FBXT_ACTIVE_UPDATE_TIMEOUT`
* `FBXT_INACTIVE_UPDATE_TIMEOUT`

These tags influence setup behavior and the resulting instance configuration.

### `ops`

`ops` is the backend operation table.

It supplies the backend callback contract through `struct fuse_operations`.

This table is not used as an eternal external reference. It is consumed during setup.

### `opssize`

`opssize` defines how much of the supplied `struct fuse_operations` is valid.

It is a compatibility boundary.

filesysbox copies only the valid prefix of the table and treats hooks beyond that size as absent.

### `udata`

`udata` is the backend private context pointer.

filesysbox stores it as backend-private state.

It is not copied.

## Setup tags and their role

The setup tag list controls instance creation.

### `FBXT_FSFLAGS`

This tag supplies filesysbox-specific setup flags, including:

* `FBXF_ENABLE_UTF8_NAMES`
* `FBXF_ENABLE_DISK_CHANGE_DETECTION`
* `FBXF_USE_INO`
* `FBXF_USE_FILL_DIR_STAT`

These flags alter practical backend expectations around naming, disk-change integration, object identity, and directory metadata.

### `FBXT_FSSM`

This tag supplies a filesystem startup message value for setup use.

### `FBXT_DOSTYPE`

This tag supplies the DOS type used for the mounted filesystem.

### `FBXT_GET_CONTEXT`

This tag controls whether filesysbox should make the FUSE-style context available through `fuse_get_context()`.

### `FBXT_ACTIVE_UPDATE_TIMEOUT`

This tag controls the active update timeout.

### `FBXT_INACTIVE_UPDATE_TIMEOUT`

This tag controls the inactive update timeout.

These timeout tags are part of instance configuration and influence runtime update behavior.

## Result

`FbxSetupFS()` returns:

* a valid `struct FbxFS *` on success
* `NULL` on failure

This is the public boundary of setup success or failure.

### Success

On success:

* a filesysbox instance exists
* the backend operation table has been normalized and installed
* backend private context has been attached
* the instance is ready for runtime through `FbxEventLoop()`

### Failure

On failure:

* no usable instance is returned
* the caller must not proceed to `FbxEventLoop()`
* partially initialized internal setup state is cleaned up before failure returns to the caller

That makes `NULL` a complete public failure boundary for setup.

## Ownership and lifetime

### Operation table lifetime

The supplied operation table is copied internally up to `opssize`.

That means the original `ops` object only needs to remain valid during the setup call itself.

### Backend private context lifetime

The `udata` pointer is stored, not copied.

That means:

* filesysbox does not become owner of the backend-private object itself
* the backend or caller remains responsible for its lifetime
* the object pointed to by `udata` must remain valid for the intended lifetime of the mounted backend

### Instance lifetime

The returned `struct FbxFS *` is the public lifecycle handle of the created instance.

It remains valid from successful setup until `FbxCleanupFS()` completes.

### Startup message lifetime

When setup is used with a valid startup message, filesysbox handles the reply path itself.

That means the caller should not separately return that same startup message through `FbxReturnMountMsg()` on the normal setup path.

## Setup-time normalization

`FbxSetupFS()` does more than simple storage.

It normalizes the supplied backend operation table.

That includes documented or observed fallback behavior such as:

* `opendir` falling back to `open`
* `releasedir` falling back to `release`
* `fgetattr` falling back to `getattr`
* `ftruncate` falling back to `truncate`

In addition, filesysbox may install internal dummy handlers for missing hooks.

This means setup transforms the supplied backend contract into the active runtime contract.

## Backend-private context during setup

The backend-private context passed through `udata` becomes the backend-facing private state available through the filesysbox-side FUSE context.

This is the public way to associate backend-owned state with the created filesysbox instance.

A backend should therefore treat `udata` as the root of its backend-private lifetime for that mounted instance.

## Relationship to startup-message handling

`FbxSetupFS()` sits at the boundary between explicit startup-message control and normal instance setup.

### When startup is inspected first

A caller may first inspect startup information through `FbxQueryMountMsg()` and then proceed to `FbxSetupFS()`.

### When startup is explicitly terminated without setup

A caller may decide not to proceed with setup and instead explicitly reply through `FbxReturnMountMsg()`.

### When setup handles startup normally

If setup is used with a valid startup message, the normal rule is:

* let `FbxSetupFS()` handle that startup message path
* do not separately reply to the same message

## Relationship to runtime

A successful `FbxSetupFS()` call is the prerequisite for runtime.

`FbxEventLoop()` requires a valid instance returned by `FbxSetupFS()`.

That means setup is not optional in the public lifecycle. It is the creation boundary of the runtime object.

## Relationship to teardown

`FbxCleanupFS()` is the public teardown counterpart of `FbxSetupFS()`.

That means:

* `FbxSetupFS()` creates the instance
* `FbxCleanupFS()` destroys it

A caller that receives a valid instance from `FbxSetupFS()` becomes responsible for later calling `FbxCleanupFS()`.

## Practical rules for callers and backend authors

A caller or backend author should follow these rules:

* provide a valid `struct fuse_operations` table
* set `opssize` correctly
* keep `udata` valid for the lifetime of the mounted backend
* use setup tags intentionally rather than implicitly
* treat a `NULL` result as complete setup failure
* call `FbxEventLoop()` only after successful setup
* call `FbxCleanupFS()` exactly for successfully created instances
* do not separately return the startup message when `FbxSetupFS()` is handling the normal setup path

## Relationship to other public contracts

`FbxSetupFS()` must be read together with:

* `FbxQueryMountMsg()` and `FbxReturnMountMsg()` for startup-message handling
* `FbxEventLoop()` for runtime
* `FbxCleanupFS()` for teardown
* `struct fuse_operations` for the backend callback contract
* `struct fuse_file_info` for handle semantics
* `struct fbx_stat` for metadata semantics

## Notes

This document describes the setup contract already made explicit through:

* the public prototype of `FbxSetupFS()`
* the documented setup tags
* reviewed setup behavior
* documented and observed callback normalization

It distinguishes deliberately between:

* setup inputs
* setup-time ownership and lifetime
* startup-message handling
* instance creation and setup failure

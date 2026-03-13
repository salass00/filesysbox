# Connection context

> Status: working draft  
> Sources: public headers, implementation review  
> Goal: explicit documentation of the public setup-time connection contract represented by `struct fuse_conn_info`

## Purpose

This document describes the public setup-time connection context represented by `struct fuse_conn_info`.

`struct fuse_conn_info` is part of backend initialization. It is presented to the backend through the `init` hook in `struct fuse_operations`.

The purpose of this document is to state explicitly:

- what `struct fuse_conn_info` is for
- which parts of it are active in the current practical contract
- which parts are currently compatibility surface
- how `volume_name` fits into backend initialization

## Public structure overview

`struct fuse_conn_info` contains these public fields:

- `proto_major`
- `proto_minor`
- `async_read`
- `max_write`
- `max_readahead`
- `reserved[27]`
- `volume_name[CONN_VOLUME_NAME_BYTES]`

The public header comment already gives a strong summary of the current practical contract:

- most fields are not used yet and are just cleared
- `volume_name` is a filesysbox addition intended for `.init()` to fill

That comment defines the practical meaning of this structure much more clearly than the raw field list alone.

## General contract

`struct fuse_conn_info` is a setup-time structure.

It is not a normal runtime object and it is not a general-purpose per-request context.

Its current practical contract is narrow:

- filesysbox prepares it before backend initialization
- the backend may inspect it in `init()`
- the backend may fill `volume_name`
- most other visible fields are currently compatibility surface and are not part of the active practical contract

This means the structure should be documented as a setup-time connection object, not as a fully active negotiation structure.

## Relationship to backend `init`

The most important place where this structure matters is the backend `init` hook.

The public callback type is:

- `void *(*init)(struct fuse_conn_info *conn)`

That means `struct fuse_conn_info` is part of backend initialization and belongs to the lifecycle stage after `FbxSetupFS()` has begun setup, but before normal runtime operation in `FbxEventLoop()`.

The practical initialization sequence is:

1. filesysbox begins setup through `FbxSetupFS()`
2. filesysbox prepares `struct fuse_conn_info`
3. backend `init()` receives `conn`
4. backend initialization may inspect or fill relevant setup-time fields
5. setup completes and normal runtime may begin

This makes `struct fuse_conn_info` a setup-time contract, not a runtime contract.

## `volume_name`

`volume_name` is the most important active field in the current practical contract.

The public header explicitly documents it as:

- a filesysbox addition
- intended for `.init()` to fill

That gives it a stronger contract role than any of the other visible fields in the structure.

### Practical meaning

`volume_name` is the backend-facing place to provide the mounted volume name during initialization.

That means:

- backend `init()` may fill `conn->volume_name`
- filesysbox treats this field as meaningful setup output
- the mounted filesystem identity on the AmigaOS side depends on this field

### Practical rule

A backend that wants to control the mounted volume name should do so through `conn->volume_name` in `init()`.

A minimal example backend should use this field explicitly.

## `proto_major` and `proto_minor`

These fields are part of the public structure, but the public header comment states that the non-`volume_name` fields are currently not used yet and are just cleared.

For current API documentation, that means:

- they exist in the public structure
- they should not be documented as active required negotiation fields in the current practical contract
- a backend should not assume that filesysbox currently assigns them active semantic weight

They belong to the visible compatibility surface of the structure.

## `async_read`, `max_write`, and `max_readahead`

These fields are also part of the public structure.

However, under the current practical contract described by the public header comment, they are currently just cleared and are not part of the active filesysbox-facing setup core.

For current API documentation, that means:

- they should be documented as present
- they should not be documented as active backend obligations or active filesysbox-controlled negotiation settings
- a backend should not rely on them as part of the currently documented practical contract

## `reserved[27]`

The `reserved` array is part of the public structure layout.

Under the current practical contract, it belongs entirely to compatibility surface.

It should not be documented as backend-controlled semantic state.

Its presence is structural, not part of the currently active backend-facing setup contract.

## Active fields in the current practical contract

The active current practical contract for `struct fuse_conn_info` is very small.

### Actively meaningful

- `volume_name`

### Present but currently not active in the documented practical contract

- `proto_major`
- `proto_minor`
- `async_read`
- `max_write`
- `max_readahead`
- `reserved[27]`

This is not speculation. It follows directly from the public header comment describing the structure.

## Minimal backend contract

For a minimal backend, the practical contract of `struct fuse_conn_info` is:

- implement `init()` if setup-time clarity is desired
- treat `conn` as a setup-time object
- fill `conn->volume_name` if the backend wants to provide a mounted volume name
- do not assume that the remaining visible fields participate in active negotiation in the current documented contract

That is the smallest explicit and defensible current contract.

## Recommended backend practice

A backend should:

- treat `struct fuse_conn_info` as setup-time input/output, not runtime state
- use `conn->volume_name` when it wants to provide the mounted volume name
- avoid building backend behavior around the other fields unless future documentation assigns them active meaning
- keep `init()` use of `conn` simple and explicit

For current documentation purposes, the safest recommendation is:

- document `volume_name` as the active field
- document the remaining fields as currently inactive compatibility surface

## Relationship to other public contracts

The connection-context contract must be read together with:

- `FbxSetupFS()`, because this structure exists only as part of setup
- `struct fuse_operations`, especially `init` and `destroy`
- the backend lifecycle contract, because `conn` belongs to setup rather than runtime
- the mounted volume identity contract, because `volume_name` influences the visible mounted filesystem name

## Notes

This document describes the current practical contract already made explicit by the public header comment for `struct fuse_conn_info`.

It distinguishes deliberately between:

- the active setup-time field `volume_name`
- the role of `init()` as the place where `conn` is consumed
- the remaining visible fields, which currently belong to compatibility surface rather than to the active documented setup contract

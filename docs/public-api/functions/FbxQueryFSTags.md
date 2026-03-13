# FbxQueryFSTags

> Status: working draft
> Sources: public headers, implementation review
> Goal: explicit documentation of the public varargs query contract of `FbxQueryFSTags()`

## Purpose

`FbxQueryFSTags()` queries information about a filesysbox-managed filesystem instance through a varargs tag interface.

It is the varargs companion to `FbxQueryFS()`.

## Synopsis

```
void FbxQueryFSTags(struct FbxFS * fs, Tag tags, ...);
```

## Public role in the lifecycle

`FbxQueryFSTags()` belongs to the query and inspection surface of the public API.

It is not part of:

* startup-message handling
* instance creation
* active runtime entry
* final teardown

It operates on an already existing filesysbox instance.

## Inputs

`FbxQueryFSTags()` takes:

* `fs`
* `tags, ...`

### `fs`

`fs` is the filesysbox instance being queried.

For the practical public contract, it must be a live instance created by `FbxSetupFS()`.

### `tags, ...`

`tags, ...` is a varargs tag list.

This varargs list defines both:

* what is being queried
* where query results are written

The currently documented public query tag is:

* `FBXT_GMT_OFFSET`

## Result

`FbxQueryFSTags()` does not return a direct query result value.

Instead, it uses the supplied varargs tags as the query and result-storage interface.

The public result model is therefore:

* the caller supplies tag-based result storage
* filesysbox fills that storage according to the supported tags

## Query model

`FbxQueryFSTags()` is the varargs form of the public instance-query API.

That means:

* it does not use a single fixed numeric selector
* it uses a varargs tag sequence
* its meaning depends on the supported public tags

For current documentation, the explicitly documented public query tag is `FBXT_GMT_OFFSET`.

## Supported documented public query tag

### `FBXT_GMT_OFFSET`

`FBXT_GMT_OFFSET` is documented as a query tag for the filesysbox query interface.

Its role is to expose the GMT or UTC offset value associated with the filesysbox instance.

This confirms that the varargs query surface is part of the public API rather than a private convenience wrapper.

## Relationship to `FbxQueryFS()`

`FbxQueryFSTags()` and `FbxQueryFS()` are two public entry forms of the same general query model.

The distinction is:

* `FbxQueryFS()` uses an explicit `struct TagItem *` list
* `FbxQueryFSTags()` uses varargs tags

This means `FbxQueryFSTags()` should be documented as a convenience form of the same instance-query surface, not as a separate semantic subsystem.

## Instance requirements

`FbxQueryFSTags()` operates on an existing filesysbox instance.

That means the practical rule is:

* call it only on an instance created by `FbxSetupFS()`
* do not treat it as a pre-setup query
* do not treat it as a startup-message helper

It is an instance-query function, not a startup-control function.

## Ownership and lifetime

`FbxQueryFSTags()` is a query interface, not an ownership-transfer interface.

That means:

* the caller owns the varargs-based result storage it supplies
* the function does not create a new public instance object
* the function does not transfer ownership of internal filesysbox state to the caller

Where query results refer to existing instance state, they should be treated as query output, not as caller-owned internals.

## Side effects

`FbxQueryFSTags()` should be treated as a read-oriented instance query.

It does not:

* create an instance
* enter runtime
* destroy an instance

Its role is inspection of an already existing instance through the public varargs tag interface.

## Relationship to lifecycle functions

`FbxQueryFSTags()` depends on instance lifetime, but it is not itself a lifecycle transition.

It belongs after successful setup and before final cleanup, whenever a live instance is available.

It does not replace:

* `FbxSetupFS()`
* `FbxEventLoop()`
* `FbxCleanupFS()`

## Minimal backend relevance

A minimal backend does not need `FbxQueryFSTags()` in order to mount and run normally.

However, it remains relevant as part of the public API because it documents the existence of a varargs query surface for instance-level information.

That makes it useful for:

* wrappers
* diagnostics
* supporting tools
* API completeness

## Practical rules for callers and backend authors

A caller or backend author should follow these rules:

* treat `FbxQueryFSTags()` as a varargs tag query function
* provide valid result storage through the supplied tags
* call it only on a live filesysbox instance
* do not treat it as a lifecycle or startup function
* do not expect ownership transfer of internal instance state

## Relationship to other public contracts

`FbxQueryFSTags()` must be read together with:

* `FbxQueryFS()` as the explicit tag-list companion API
* `FbxSetupFS()` because it requires an existing instance
* the lifecycle documentation because it belongs after instance creation rather than before it

## Notes

This document describes the public varargs query contract already made explicit through:

* the public prototype of `FbxQueryFSTags()`
* the documented public query tag `FBXT_GMT_OFFSET`
* the relationship between the explicit tag-list and varargs query APIs

It distinguishes deliberately between:

* varargs instance query
* explicit tag-list instance query
* startup and lifecycle functions


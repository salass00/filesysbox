# FbxQueryFS

> Status: working draft
> Sources: public headers, implementation review
> Goal: explicit documentation of the public query contract of `FbxQueryFS()`

## Purpose

`FbxQueryFS()` queries information about a filesysbox-managed filesystem instance through a tag-list interface.

It is a public helper for retrieving selected instance-related information without exposing internal structure directly.

## Synopsis

```
void FbxQueryFS(struct FbxFS * fs, const struct TagItem * tags);
```

## Public role in the lifecycle

`FbxQueryFS()` belongs to the query and inspection surface of the public API.

It is not part of:

* startup-message handling
* instance creation
* active runtime entry
* final teardown

It operates on an already existing filesysbox instance.

## Inputs

`FbxQueryFS()` takes two public inputs:

* `fs`
* `tags`

### `fs`

`fs` is the filesysbox instance being queried.

For the practical public contract, it must be a live instance created by `FbxSetupFS()`.

### `tags`

`tags` is a `struct TagItem *` query list.

This list defines both:

* what is being queried
* where query results are written

The currently documented public query tag is:

* `FBXT_GMT_OFFSET`

This means the currently documented public query interface is tag-based, not selector-based.

## Result

`FbxQueryFS()` does not return a direct query result value.

Instead, it uses the supplied tag list as the query and result-storage interface.

The public result model is therefore:

* the caller supplies tag-based result storage
* filesysbox fills that storage according to the supported tags

## Query model

`FbxQueryFS()` is a tag-list query API.

That means:

* the caller does not ask for one single fixed attribute through a numeric selector
* the caller passes a tag list describing the query
* the meaning of the call depends on the supported public tags

For current documentation, the explicitly documented public tag is `FBXT_GMT_OFFSET`.

## Supported documented public query tag

### `FBXT_GMT_OFFSET`

`FBXT_GMT_OFFSET` is documented as a query tag for `FbxQueryFS()`.

Its role is to expose the GMT or UTC offset value associated with the filesysbox instance.

For public API purposes, this means:

* `FbxQueryFS()` already has at least one documented instance-query use
* the query model is not hypothetical; it is part of the public API surface

## Instance requirements

`FbxQueryFS()` operates on an existing filesysbox instance.

That means the practical rule is:

* call `FbxQueryFS()` only with an instance created by `FbxSetupFS()`
* do not treat it as a pre-setup function
* do not treat it as an alternative to startup-message inspection

It belongs to post-setup instance inspection.

## Ownership and lifetime

`FbxQueryFS()` is a query interface, not an ownership-transfer interface.

That means:

* the caller owns the supplied tag list and result storage
* the function does not create a new public instance object
* the function does not transfer ownership of internal filesysbox state to the caller

Where query results refer to existing instance state, they should be treated as query output, not as caller-owned instance internals.

## Side effects

`FbxQueryFS()` should be treated as a read-oriented instance query.

It does not:

* create an instance
* enter runtime
* destroy an instance

Its role is inspection of an already existing instance through the public tag interface.

## Relationship to `FbxQueryFSTags()`

`FbxQueryFS()` is the explicit `struct TagItem *` form of the public query interface.

`FbxQueryFSTags()` is the varargs companion to the same general query surface.

The distinction is:

* `FbxQueryFS()` uses an explicit tag list
* `FbxQueryFSTags()` uses varargs tags

The underlying public role is the same: instance query through tags.

## Relationship to lifecycle functions

`FbxQueryFS()` should be read together with lifecycle functions, but it is not itself a lifecycle transition.

It depends on an instance already existing, which means it is downstream of `FbxSetupFS()`.

It does not replace:

* `FbxSetupFS()`
* `FbxEventLoop()`
* `FbxCleanupFS()`

It is an inspection function for the already created instance.

## Minimal backend relevance

A minimal backend does not need `FbxQueryFS()` in order to mount and run normally.

However, it remains relevant as part of the public API because it documents that filesysbox exposes instance-level information through tags.

That makes it useful for:

* wrappers
* diagnostics
* supporting tools
* API completeness

## Practical rules for callers and backend authors

A caller or backend author should follow these rules:

* treat `FbxQueryFS()` as a tag-list query function
* provide valid result storage through the supplied tags
* call it only on a live filesysbox instance
* do not treat it as a lifecycle or startup function
* do not expect ownership transfer of internal instance state

## Relationship to other public contracts

`FbxQueryFS()` must be read together with:

* `FbxQueryFSTags()` as the varargs companion API
* `FbxSetupFS()` because it requires an existing instance
* the lifecycle documentation because it belongs after instance creation rather than before it

## Notes

This document describes the public query contract already made explicit through:

* the public prototype of `FbxQueryFS()`
* the documented public query tag `FBXT_GMT_OFFSET`
* the tag-based query shape of the API

It distinguishes deliberately between:

* tag-based instance query
* startup-message inspection
* lifecycle control functions


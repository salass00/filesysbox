# FbxReturnMountMsg

> Status: working draft
> Sources: public headers, implementation review
> Goal: explicit documentation of the public startup-reply contract of `FbxReturnMountMsg()`

## Purpose

`FbxReturnMountMsg()` explicitly replies to a filesysbox startup or mount message.

It is the public helper used when the caller needs to complete the startup-message path directly instead of letting `FbxSetupFS()` do that work as part of normal setup.

## Synopsis

```
void FbxReturnMountMsg(struct Message * msg, SIPTR r1, SIPTR r2);
```

## Public role in the lifecycle

`FbxReturnMountMsg()` belongs to the startup-message phase of the public lifecycle.

It is not part of normal mounted runtime.

It does not:

* create a filesysbox instance
* enter runtime
* destroy an instance

Its role is specific and narrow:

* complete the startup-message reply path explicitly

## Inputs

`FbxReturnMountMsg()` takes three public inputs:

* `msg`
* `r1`
* `r2`

### `msg`

`msg` is the startup or mount message that should be replied to.

### `r1`

`r1` is the primary reply value returned through the startup-message path.

### `r2`

`r2` is the secondary reply or error value returned through the startup-message path.

Together, `r1` and `r2` form the explicit reply result for the startup path.

## Result

`FbxReturnMountMsg()` does not return a result value.

Its public effect is to complete the startup-message reply path for the supplied message.

That means the important effect is not a function return value, but the completion of startup-message handling itself.

## Relationship to `FbxQueryMountMsg()`

`FbxReturnMountMsg()` and `FbxQueryMountMsg()` belong to the same lifecycle phase, but they do different work.

* `FbxQueryMountMsg()` inspects startup context
* `FbxReturnMountMsg()` replies to the startup message

This distinction is part of the public lifecycle and should be documented explicitly.

## Relationship to `FbxSetupFS()`

`FbxSetupFS()` can handle the startup-message path itself when setup proceeds normally with a valid startup message.

That means the practical rule is:

* if setup is being completed through `FbxSetupFS()`, the caller should not separately call `FbxReturnMountMsg()` for the same startup path
* `FbxReturnMountMsg()` is for explicit reply handling when the caller is not relying on `FbxSetupFS()` to complete startup

This is one of the most important lifecycle boundaries around the function.

## Startup-path completion

Calling `FbxReturnMountMsg()` completes the startup-message reply path.

That means the startup phase for that message is finished.

After that point, the caller should no longer treat startup-context pointers obtained earlier as if they were still live long-term objects.

If startup-context data is needed after the reply is sent, it should have been copied before calling `FbxReturnMountMsg()`.

## Relationship to instance lifetime

`FbxReturnMountMsg()` does not create a `struct FbxFS *` instance.

That means:

* no filesysbox instance lifetime begins through this function
* it belongs entirely to pre-instance startup control flow

If the caller wants a mounted filesysbox instance, that requires `FbxSetupFS()`.

## Typical usage patterns

### Explicit startup termination without setup

A caller may:

1. inspect startup context through `FbxQueryMountMsg()`
2. decide not to continue with normal setup
3. explicitly reply through `FbxReturnMountMsg()`

This path ends before instance creation.

### Normal setup path

A caller may instead:

1. inspect startup context if needed
2. call `FbxSetupFS()`
3. allow setup to handle the startup-message reply path itself

In that case, the caller should not separately use `FbxReturnMountMsg()` for the same setup path.

## Minimal backend usage

A minimal backend will usually not need to call `FbxReturnMountMsg()` explicitly on the normal success path.

It becomes relevant when the caller wants to:

* reject startup explicitly
* terminate the startup path before instance creation
* handle startup control flow outside normal `FbxSetupFS()` instance creation

## Practical rules for callers and backend authors

A caller or backend author should follow these rules:

* use `FbxReturnMountMsg()` only for explicit startup-message reply handling
* do not use it as part of normal runtime behavior
* do not call it again for a startup path already consumed by `FbxSetupFS()`
* copy startup-context data first if it must survive completion of the startup path

## Relationship to other public contracts

`FbxReturnMountMsg()` must be read together with:

* `FbxQueryMountMsg()` for startup-context inspection
* `FbxSetupFS()` for normal startup that creates an instance
* the lifecycle document for the distinction between startup handling and instance lifetime

## Notes

This document describes the startup-reply contract already made explicit through:

* the public prototype of `FbxReturnMountMsg()`
* the relationship between startup-message handling and normal setup
* the public lifecycle distinction between startup control and instance creation

It distinguishes deliberately between:

* startup-message inspection
* explicit startup-message reply handling
* later instance creation through `FbxSetupFS()`


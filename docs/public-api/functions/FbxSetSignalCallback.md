# FbxSetSignalCallback

> Status: working draft
> Sources: public headers, implementation review
> Goal: explicit documentation of the public signal-callback registration contract of `FbxSetSignalCallback()`

## Purpose

`FbxSetSignalCallback()` installs or updates a signal-related callback for a filesysbox-managed filesystem instance.

It is part of the optional public integration surface for callers that want filesysbox to notify them through a signal callback during the lifetime of a live instance.

## Synopsis

```
void FbxSetSignalCallback(struct FbxFS * fs, FbxSignalCallbackFunc func, ULONG signals);
```

## Public role in the lifecycle

`FbxSetSignalCallback()` is not a lifecycle transition.

It does not:

* create an instance
* begin runtime
* destroy an instance

It is a configuration function that applies to an already existing filesysbox instance.

## Inputs

`FbxSetSignalCallback()` takes:

* `fs`
* `func`
* `signals`

### `fs`

`fs` is the filesysbox instance whose signal-callback behavior is being configured.

For the practical public contract, it must be a live instance created by `FbxSetupFS()`.

### `func`

`func` is the signal callback function.

This callback is supplied by the caller.

### `signals`

`signals` is the signal mask associated with the callback registration.

This mask is part of the public registration contract.

## Result

`FbxSetSignalCallback()` does not return a result value.

Its public effect is to configure signal-callback behavior for the supplied instance.

## Callback model

The public callback type is:

* `typedef STDARGS void (*FbxSignalCallbackFunc)(ULONG matching_signals);`

That means the public callback contract is explicitly:

* callback function returns `void`
* callback receives the matching signal mask as its argument

There is no separate callback user-data argument in the public API.

This is part of the documented callback shape and should be stated explicitly.

## Instance requirements

`FbxSetSignalCallback()` operates on an existing filesysbox instance.

The practical rule is:

* call it only on an instance created by `FbxSetupFS()`
* treat it as post-setup instance configuration
* do not treat it as a startup-message function

## Ownership and lifetime

The callback function pointer is caller-supplied.

That means:

* filesysbox does not become owner of the callback function itself
* the caller remains responsible for ensuring that the callback target remains valid while registered

Because there is no public callback user-data argument, any callback-side state must be obtained through caller-managed external context rather than through an API-provided user-data pointer.

## Relationship to runtime

`FbxSetSignalCallback()` configures behavior associated with a live filesysbox instance.

That means its practical role belongs to the period after setup and before final cleanup.

It is runtime-related configuration, not instance creation.

## Relationship to other callback functions

`FbxSetSignalCallback()` belongs to the optional callback and integration part of the public API.

It is related to:

* `FbxInstallTimerCallback()`
* `FbxUninstallTimerCallback()`
* `FbxSignalDiskChange()`

These functions all extend the public integration surface around a live filesysbox instance.

## Public callback semantics already defined

The public API already defines these signal-callback facts explicitly:

* registration is instance-bound
* the callback type is fixed
* the callback receives a matching signal mask
* there is no separate user-data pointer in the public API

This is the currently documented signal-callback contract.

## Scope boundary

This document describes the public registration interface of `FbxSetSignalCallback()`.

It does not define a complete execution-context or scheduler model for signal callback invocation.

That is a scope boundary of this function document, not an unresolved public prototype question.

## Minimal backend relevance

A minimal backend does not need `FbxSetSignalCallback()` in order to mount and run normally.

This function belongs to optional integration behavior rather than to the minimal required runtime path.

Its importance is therefore:

* API completeness
* integration hooks for wrappers or advanced users
* optional host-side coordination

## Practical rules for callers and backend authors

A caller or backend author should follow these rules:

* call `FbxSetSignalCallback()` only on a live filesysbox instance
* keep the callback target valid while it is registered
* do not expect a public user-data pointer to be stored alongside the callback
* treat this function as optional integration configuration rather than as part of the minimal lifecycle

## Relationship to other public contracts

`FbxSetSignalCallback()` must be read together with:

* `FbxSetupFS()` because it requires an existing instance
* `FbxEventLoop()` because the callback belongs to live instance behavior
* `FbxCleanupFS()` because registration lifetime cannot outlive the instance
* `FbxInstallTimerCallback()` and `FbxUninstallTimerCallback()` because they form the related callback integration surface

## Notes

This document describes the signal-callback registration contract already made explicit through:

* the public prototype of `FbxSetSignalCallback()`
* the public callback typedef `FbxSignalCallbackFunc`
* the instance-bound nature of the function

It distinguishes deliberately between:

* public registration shape
* callback ownership and lifetime
* broader runtime integration behavior


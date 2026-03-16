# FbxInstallTimerCallback

> Status: working draft
> Sources: public headers, implementation review
> Goal: explicit documentation of the public timer-callback installation contract of `FbxInstallTimerCallback()`

## Purpose

`FbxInstallTimerCallback()` installs a timer-related callback for a filesysbox-managed filesystem instance.

It is part of the optional public integration surface for callers that want timer-driven callback behavior associated with a live filesysbox instance.

## Synopsis

```
struct FbxTimerCallbackData * FbxInstallTimerCallback(
struct FbxFS * fs,
FbxTimerCallbackFunc func,
ULONG period
);
```

## Public role in the lifecycle

`FbxInstallTimerCallback()` is not a lifecycle transition.

It does not:

* create an instance
* begin runtime
* destroy an instance

It is a configuration function that applies to an already existing filesysbox instance.

## Inputs

`FbxInstallTimerCallback()` takes:

* `fs`
* `func`
* `period`

### `fs`

`fs` is the filesysbox instance whose timer-callback behavior is being configured.

For the practical public contract, it must be a live instance created by `FbxSetupFS()`.

### `func`

`func` is the timer callback function.

This callback is supplied by the caller.

### `period`

`period` is the callback period.

This period is part of the public installation contract.

## Result

`FbxInstallTimerCallback()` returns a `struct FbxTimerCallbackData *`.

This returned object is the public registration handle for the installed timer callback.

Its practical meaning is:

* the callback has been installed for the supplied instance
* the returned handle identifies that installed callback registration
* the handle is later used with `FbxUninstallTimerCallback()`

## Callback model

The public callback type is:

* `typedef STDARGS void (*FbxTimerCallbackFunc)(void);`

That means the public timer callback contract is explicitly:

* callback function returns `void`
* callback takes no public arguments

There is no public callback user-data argument in the API.

## Registration-handle model

`FbxInstallTimerCallback()` differs from `FbxSetSignalCallback()` in one important way:

* it returns an explicit registration handle

That means timer callback installation is modeled as:

* install a callback
* receive a registration handle
* later remove that specific registration through `FbxUninstallTimerCallback()`

This is the central public contract of the function.

## Instance requirements

`FbxInstallTimerCallback()` operates on an existing filesysbox instance.

The practical rule is:

* call it only on an instance created by `FbxSetupFS()`
* treat it as post-setup instance configuration
* do not treat it as a startup-message function

## Ownership and lifetime

The callback function pointer is caller-supplied.

That means:

* filesysbox does not become owner of the callback function itself
* the caller remains responsible for keeping the callback target valid while installed

The returned `struct FbxTimerCallbackData *` is the public registration handle of the installed callback.

That handle remains part of the callback-registration contract until it is removed through `FbxUninstallTimerCallback()` or until the instance lifetime ends.

## Relationship to runtime

`FbxInstallTimerCallback()` configures timer-driven behavior associated with a live filesysbox instance.

That places it in the period after successful setup and before final cleanup.

It is runtime-related configuration, not instance creation.

## Relationship to `FbxUninstallTimerCallback()`

`FbxInstallTimerCallback()` and `FbxUninstallTimerCallback()` form one public registration lifecycle.

The contract is:

* install through `FbxInstallTimerCallback()`
* receive a `struct FbxTimerCallbackData *` registration handle
* uninstall through `FbxUninstallTimerCallback()` using that handle

That registration-handle relationship is part of the explicit public API shape.

## Public timer-callback semantics already defined

The public API already defines these timer-callback facts explicitly:

* registration is instance-bound
* the callback type is fixed
* the callback takes no public arguments
* installation takes a `period`
* installation returns a registration handle
* there is no separate public user-data pointer

This is the currently documented timer-installation contract.

## Scope boundary

This document describes the public installation interface of `FbxInstallTimerCallback()`.

It does not define a complete scheduler model, dispatch model, or callback execution-context model.

That is a scope boundary of this function document, not a missing statement about the public prototype.

## Minimal backend relevance

A minimal backend does not need `FbxInstallTimerCallback()` in order to mount and run normally.

This function belongs to optional integration behavior rather than to the minimal required runtime path.

Its importance is therefore:

* API completeness
* advanced integration behavior
* wrapper or host-side timer-driven coordination

## Practical rules for callers and backend authors

A caller or backend author should follow these rules:

* call `FbxInstallTimerCallback()` only on a live filesysbox instance
* keep the callback target valid while it is installed
* keep track of the returned registration handle
* use that handle later with `FbxUninstallTimerCallback()`
* do not expect a public user-data pointer to be stored with the registration

## Relationship to other public contracts

`FbxInstallTimerCallback()` must be read together with:

* `FbxSetupFS()` because it requires an existing instance
* `FbxEventLoop()` because it belongs to live instance behavior
* `FbxCleanupFS()` because registration lifetime cannot outlive the instance
* `FbxUninstallTimerCallback()` because uninstallation depends on the returned registration handle
* `FbxSetSignalCallback()` because both functions belong to the optional callback integration surface

## Notes

This document describes the timer-callback installation contract already made explicit through:

* the public prototype of `FbxInstallTimerCallback()`
* the public callback typedef `FbxTimerCallbackFunc`
* the returned registration-handle type `struct FbxTimerCallbackData *`

It distinguishes deliberately between:

* public installation shape
* callback ownership and lifetime
* registration-handle semantics
* broader runtime integration behavior


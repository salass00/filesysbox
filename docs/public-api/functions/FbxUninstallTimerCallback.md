# FbxUninstallTimerCallback

> Status: working draft
> Sources: public headers, implementation review
> Goal: explicit documentation of the public timer-callback removal contract of `FbxUninstallTimerCallback()`

## Purpose

`FbxUninstallTimerCallback()` removes a previously installed timer callback from a filesysbox-managed filesystem instance.

It is the public counterpart to `FbxInstallTimerCallback()`.

## Synopsis

```
void FbxUninstallTimerCallback(struct FbxFS * fs, struct FbxTimerCallbackData * cb);
```

## Public role in the lifecycle

`FbxUninstallTimerCallback()` is not a lifecycle transition.

It does not:

* create an instance
* begin runtime
* destroy an instance

It is a configuration function that applies to an already existing filesysbox instance.

## Inputs

`FbxUninstallTimerCallback()` takes:

* `fs`
* `cb`

### `fs`

`fs` is the filesysbox instance whose timer-callback registration is being modified.

For the practical public contract, it must be a live instance created by `FbxSetupFS()`.

### `cb`

`cb` is the registration handle of a previously installed timer callback.

This is the `struct FbxTimerCallbackData *` value returned by `FbxInstallTimerCallback()`.

That makes `cb` the public identifier of the timer callback registration being removed.

## Result

`FbxUninstallTimerCallback()` does not return a result value.

Its public effect is to remove the installed timer-callback registration identified by `cb` from the supplied instance.

## Registration-handle model

`FbxUninstallTimerCallback()` is defined by the registration-handle contract established by `FbxInstallTimerCallback()`.

The public model is:

* installation returns a `struct FbxTimerCallbackData *` handle
* uninstallation consumes that handle to remove the corresponding callback registration

This is the central public contract of the function.

## Instance requirements

`FbxUninstallTimerCallback()` operates on an existing filesysbox instance.

The practical rule is:

* call it only on an instance created by `FbxSetupFS()`
* use it only with a registration handle that came from `FbxInstallTimerCallback()`
* treat it as post-setup instance configuration rather than as lifecycle control

## Ownership and lifetime

`cb` is the public registration handle of an installed timer callback.

It is not an independently meaningful instance object outside that registration relationship.

That means:

* `cb` identifies an installed callback registration
* `FbxUninstallTimerCallback()` uses that handle to remove the registration
* the registration cannot outlive the filesysbox instance to which it belongs

The callback function itself remains caller-supplied and caller-owned.

## Relationship to runtime

`FbxUninstallTimerCallback()` belongs to live-instance configuration.

It operates during the lifetime of an already existing filesysbox instance.

That places it after successful setup and before final cleanup.

## Relationship to `FbxInstallTimerCallback()`

`FbxUninstallTimerCallback()` is the public removal half of the timer-callback registration lifecycle.

The explicit contract is:

* install through `FbxInstallTimerCallback()`
* receive a `struct FbxTimerCallbackData *` handle
* remove that callback registration through `FbxUninstallTimerCallback()`

This direct handle-based relationship is part of the public API shape.

## Public timer-removal semantics already defined

The public API already defines these removal facts explicitly:

* removal is instance-bound
* removal uses a `struct FbxTimerCallbackData *` registration handle
* the handle comes from prior installation through `FbxInstallTimerCallback()`
* removal itself returns no value

This is the currently documented timer-removal contract.

## Scope boundary

This document describes the public removal interface of `FbxUninstallTimerCallback()`.

It does not define a complete dispatch-order or execution-order model for in-flight callback execution.

That is a scope boundary of this function document, not a missing statement about the public removal API.

## Minimal backend relevance

A minimal backend does not need `FbxUninstallTimerCallback()` in order to mount and run normally.

This function belongs to optional integration behavior rather than to the minimal required runtime path.

Its importance is therefore:

* API completeness
* advanced integration behavior
* wrapper or host-side management of timer-driven callback registrations

## Practical rules for callers and backend authors

A caller or backend author should follow these rules:

* call `FbxUninstallTimerCallback()` only on a live filesysbox instance
* use only a registration handle that came from `FbxInstallTimerCallback()`
* treat removal as part of timer-callback registration management, not as a lifecycle function
* do not treat `cb` as a general-purpose instance object outside the timer-registration relationship

## Relationship to other public contracts

`FbxUninstallTimerCallback()` must be read together with:

* `FbxInstallTimerCallback()` because it consumes the installation handle returned there
* `FbxSetupFS()` because it requires an existing instance
* `FbxEventLoop()` because it belongs to live instance behavior
* `FbxCleanupFS()` because registration lifetime cannot outlive the instance

## Notes

This document describes the timer-callback removal contract already made explicit through:

* the public prototype of `FbxUninstallTimerCallback()`
* the returned registration-handle type from `FbxInstallTimerCallback()`
* the instance-bound nature of timer callback registration

It distinguishes deliberately between:

* lifecycle functions
* optional timer-registration functions
* registration-handle semantics


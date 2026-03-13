# filesysbox public API draft documentation

> Status: working draft  
> Purpose: consolidated draft documentation of the public filesysbox API

This directory contains draft documentation for the public filesysbox API.

Its primary goal is to describe the public backend-facing contracts of filesysbox explicitly and consistently enough that they can later be turned into maintainer-friendly API documentation.

A future example backend remains a secondary follow-up task built on top of this documentation work.

## Primary goal

The primary goal of this package is:

- explicit documentation of the public filesysbox API

This includes:

- lifecycle documentation
- backend operation-table documentation
- metadata documentation
- handle documentation
- connection-context documentation
- error-model documentation
- function-level API notes for the public entry points and helpers

## Secondary goal

A future example backend is a later follow-up goal.

It is intentionally downstream of the API documentation effort.

That means:

- API documentation comes first
- an example backend will later be based on the documented API contracts
- this package should not depend on example-backend work in order to stand on its own as API documentation

## What this package documents

This package documents the public contracts of filesysbox as exposed through:

- public headers
- public function prototypes
- reviewed implementation paths that define practical public behavior

The focus is on documenting what backend authors, wrapper authors, and maintainers need to know about the visible API surface.

## Recommended reading order

For the main API contracts, read in this order:

1. `10-lifecycle.md`
2. `20-backend-operation-table.md`
3. `30-metadata-contract.md`
4. `40-handle-contract.md`
5. `50-connection-context.md`
6. `60-error-model.md`

After that, read the public function documents under `functions/`.

## Core documents

### Lifecycle

- `10-lifecycle.md`  
  Public lifecycle of a filesysbox-managed filesystem instance.

### Backend-facing structure and contract documents

- `20-backend-operation-table.md`  
  Public backend callback contract through `struct fuse_operations`.

- `30-metadata-contract.md`  
  Public metadata contract through `struct fbx_stat`.

- `40-handle-contract.md`  
  Public handle contract through `struct fuse_file_info`.

- `50-connection-context.md`  
  Public setup-time connection contract through `struct fuse_conn_info`.

- `60-error-model.md`  
  Practical public error model visible through setup, runtime checks, current-volume handling, and backend interaction.

## Function documents

### Lifecycle and startup functions

- `functions/FbxSetupFS.md`
- `functions/FbxEventLoop.md`
- `functions/FbxCleanupFS.md`
- `functions/FbxQueryMountMsg.md`
- `functions/FbxReturnMountMsg.md`

### Instance query and integration functions

- `functions/FbxQueryFS.md`
- `functions/FbxQueryFSTags.md`
- `functions/FbxSetSignalCallback.md`
- `functions/FbxInstallTimerCallback.md`
- `functions/FbxUninstallTimerCallback.md`
- `functions/FbxSignalDiskChange.md`

### Helper and version-query functions

- `functions/FbxGetSysTime.md`
- `functions/FbxGetUpTime.md`
- `functions/FbxVersion.md`
- `functions/FbxFuseVersion.md`
- `functions/FbxCopyStringBSTRToC.md`
- `functions/FbxCopyStringCToBSTR.md`

## Documentation style of this package

These documents aim to be:

- explicit
- structural
- backend-facing
- consistent across files
- based on public API shape and reviewed implementation behavior

They are not meant to be loose project notes.

They are draft API documentation.

## Scope of the current work

The current work focuses on making the public API documentation complete in substance.

That means:

- answering open technical questions
- folding the answers into the main documents
- documenting the visible contracts explicitly
- keeping the API notes useful even before later wording cleanup or PR-focused polishing

## What is not the main focus right now

The current package is not primarily focused on:

- stylistic PR polishing
- final autodoc formatting
- building the first example backend yet

Those are later steps.

The present step is to complete the API documentation itself.

## Relationship to a future example backend

A future example backend is still useful, but it is not the current main deliverable.

Its role is:

- to consume the documented API contracts
- to demonstrate the API after the documentation base is complete
- to act as a later executable companion to the API notes

That makes the example backend a downstream user of this documentation package rather than a prerequisite for it.

## Current state of the package

At this stage, the package already contains:

- explicit core contract documents
- lifecycle documentation
- function-by-function public API notes
- a consistent separation between structure contracts and function contracts

This means the package can now be treated as a draft API documentation set rather than as a loose exploratory notebook.

## Expected later work

Later work may include:

- wording unification across all files
- conversion into a more maintainers-facing PR tone
- conversion into clearer autodoc-style wording where appropriate
- example-backend work built on top of the documented API

Those are follow-up steps, not the current primary task.

## Intended outcome

The intended outcome of this package is:

- a complete draft of the public filesysbox API documentation
- a stable basis for maintainer-friendly documentation PRs
- a solid base for later example-backend work


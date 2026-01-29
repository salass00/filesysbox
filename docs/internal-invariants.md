# filesysbox internal invariants (developer notes)

This document describes internal invariants and assumptions in `filesysbox`.
It is intended to help maintainers and contributors reason about correctness and avoid regressions.
Documentation only: no behavior is defined here; it captures what the current code relies on.

## 1. Terminology

- **FS instance / connection**: A running filesysbox filesystem instance with its internal state and callbacks.
- **Entry**: An internal representation of a directory entry or path-related object cached by filesysbox.
- **Lock**: AmigaDOS lock representation used for directory traversal and object identity.
- **Notify node**: Internal wrapper for an `Exec`/DOS `NotifyRequest` registered by clients.
- **Packet**: AmigaDOS packet routed to the filesystem handler and translated into callback operations.

## 2. High-level lifecycle

1. **Setup**
   - Filesysbox is initialized and the filesystem instance state is created.
   - Callback table (FUSE-like ops) is registered.
2. **Event loop**
   - Packets are processed, translated, and dispatched.
   - Notifications and timeouts are handled.
3. **Cleanup / shutdown**
   - Resources owned by the filesystem instance are released.
   - Shutdown should not leave stale locks/notify nodes behind.

Invariant: resources created during Setup must be owned by the FS instance and released during Cleanup.

## 3. Ownership rules

This section summarizes who allocates/frees which objects.

### 3.1 Locks
- Locks created/managed by filesysbox must be released through the corresponding FREE/UNLOCK paths.
- Any internal list node associated with a lock must be freed exactly once.
- If an internal lock owns a memory pool, the pool must be deleted before freeing the lock.

Invariant: a lock pointer identity is used as the "handle"; not-found checks must not silently succeed.

### 3.2 Notify nodes
- A `NotifyRequest` may carry an internal pointer (e.g. `nr->nr_notifynode`) which is considered owned by filesysbox.
- When removing a notify request, filesysbox must:
  - locate the corresponding internal notify list node
  - unlink it
  - free the list node
  - free the notify node
  - reset request fields (`nr_MsgCount`, `nr_notifynode`) to a safe state

Invariant: notification bookkeeping uses a dedicated notify list; it must not mix with lock list management.

### 3.3 Strings / path buffers
- Name and path buffers must be treated as bounded.
- Prefer bounded copy helpers (e.g. `strlcpy`/UTF-8 helpers) over unbounded copies.
- UTF-8 validation (when enabled) must reject invalid input rather than passing it deeper.

Invariant: do not assume input strings are valid UTF-8 unless explicitly checked.

## 4. List iteration invariants (sentinel lists)

Many internal lists are implemented as Exec-style linked lists with a sentinel tail.
Typical iteration pattern:

- Start at `list.mlh_Head`
- Continue while `node->ln_Succ != NULL`

Invariant: with this pattern, **`node` will not become NULL** to indicate "not found".
Therefore, "not found" must be tracked explicitly (e.g. a `found` flag) rather than checking `node == NULL`.

## 5. Locking and ordering (conceptual)

(Keep this section in sync with the code if new semaphores/locks are introduced.)

- Avoid holding internal locks while calling out into user-provided filesystem callbacks unless the code explicitly guarantees reentrancy.
- Define and keep a strict lock ordering if multiple locks are taken in one code path.

Invariant: lock ordering must be stable to avoid deadlocks, especially around notifications and unmount/cleanup.

## 6. Error model

- Packet handlers typically report results via `(r1, r2)`:
  - `r1`: `DOSTRUE` / `DOSFALSE` (success/failure)
  - `r2`: additional error (often an AmigaDOS `ERROR_*` code)
- Not-found conditions must return failure (not success).

Invariant: operations that cannot locate the referenced internal object must not return success.

## 7. Notes for filesystem implementers (callback contract)

Filesysbox routes AmigaDOS actions into FUSE-like callbacks.

- Read-only filesystems should clearly reject write operations with an appropriate error.
- If a callback is not implemented, filesysbox should return a consistent error rather than behaving inconsistently.

Invariant: callbacks must not free or invalidate internal handles owned by filesysbox unless explicitly documented.

## 8. Documentation scope

This file documents existing assumptions. It should be updated when:
- list management strategy changes
- ownership of locks/notify nodes changes
- new locking primitives are introduced
- error model changes


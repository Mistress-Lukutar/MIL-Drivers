# AGENTS.md — Guide for AI Coding Agents

This file tells AI coding agents how to work with the **MIL Drivers**
repository. Read it fully before making any change.

## What this project is

**MIL Drivers** is the single source of truth for hardware abstraction
layer (HAL) drivers for Milandr microcontrollers. All modules carry the
`MIL_` prefix and are plain C99 with CMSIS-style register access.

Multiple firmware projects (e.g. `xBPR`, `Multicell Battery Monitor`)
consume these drivers through a sync tool (`tools/update_drivers.py`)
and a `drivers.lock` pin file. **Driver code must never be edited inside
a consuming project** — fixes and features land here first and propagate
to projects via the sync tool. If you find yourself editing a copy of a
driver inside a project repository, stop and move the change here instead.

### Layout

```
Drivers/
└── <Platform>/               # e.g. MDR1986BE9x
    └── MIL_<Module>/         # one directory per module
        ├── MIL_<Module>.c
        └── MIL_<Module>.h
tools/
└── update_drivers.py         # sync tool used by consuming projects
```

- One platform per directory under `Drivers/`. Currently only
  `MDR1986BE9x` exists; new MCU families get their own sibling directory.
- One module per directory, exactly one `.c` + `.h` pair, named after the
  module. No extra files unless the module genuinely needs them.

## Hard rules for agents

1. **Use the project C style guide.** When the
   [lukutar-c-style-guide](C:\Users\oppp0\.zcode\skills\lukutar-c-style-guide\SKILL.md)
   skill is available in your environment, you **must** load and follow it
   for any code you write or modify in this repository. It overrides your
   default habits.
2. **Format with clang-format.** The `.clang-format` at the repo root is
   authoritative (WebKit-based, 2-space indent, 150-column limit,
   left pointer alignment). Run it on every file you touch:
   ```bash
   find Drivers -name "*.c" -o -name "*.h" | xargs clang-format -i
   ```
3. **Do not break the public API silently.** Consuming projects pin this
   repo by tag. A breaking change without a MAJOR version bump and a clear
   changelog note will break their builds.
4. **Never introduce dependencies on a specific consuming project.**
   Drivers may depend on CMSIS device headers and on other `MIL_` modules
   (e.g. `MIL_I2C` uses `MIL_Time`), nothing else.

## Doxygen headers — mandatory

Every `.c` and `.h` file must start with a Doxygen header block:

```c
/**
 * @file MIL_Module.c
 * @brief One-line description of the module
 * @author Author Name
 * @date YYYY-MM-DD
 * @version vMAJOR.MINOR.PATCH
 */
```

### When you modify a file, you MUST update

- **`@date`** — set to the current date (`YYYY-MM-DD`) in every file you changed.
- **`@version`** — bump according to semver, per module (see below). The
  `.c` and `.h` of the same module must carry the **same** version. If you
  change only one of them, still align both headers.
- **Doxygen comments** — update the documentation of every function whose
  behavior, signature, or contract changed. New public functions require a
  full Doxygen block (`@brief`, `@param`, `@retval`, `@note` where useful).

### Versioning rules (per module)

| Change | Bump | Example |
|--------|------|---------|
| Breaking API change (removed/renamed function, changed signature, changed struct layout in an incompatible way) | MAJOR | v2.1.0 → v3.0.0 |
| New backward-compatible feature | MINOR | v2.1.0 → v2.2.0 |
| Bugfix, no API change | PATCH | v2.1.0 → v2.1.1 |

Also update the module version in the table in `README.md`.

## Coding conventions (summary)

| Entity | Convention | Example |
|--------|-----------|---------|
| Public functions | `MIL_<Module>_PascalCase` | `MIL_UART_SendIT()` |
| Static (private) functions | `_camelCase` | `_waitTransmission()` |
| Variables | `snake_case` | `tx_tail` |
| Constants / macros | `UPPER_CASE` | `MIL_TIME_CYCLES_PER_US` |
| Public types | `MIL_<Module>_PascalCase_t` | `MIL_UART_HandleTypeDef` |
| Enum values | `MIL_<MODULE>_UPPER_CASE` | `MIL_UART_MODE_FULL_DUPLEX` |

- Keep functions small and single-purpose; put static helpers above their
  first use.
- Comment **why**, not what — especially around hardware sequencing
  (register write order, barriers, interrupt masking). These drivers
  contain hard-won workarounds; do not "clean them up" without
  understanding them.

## Hardware-safety rules

These drivers run on bare metal, often in interrupt context. When editing:

- **Interrupt masking**: any operation that leaves a peripheral in a
  non-reentrant state (e.g. EEPROM with `CON=1`) must wrap the critical
  section in PRIMASK save/restore.
- **Memory barriers**: after clearing interrupt flags / before enabling
  IRQs, use `__DSB()` so writes are committed in order. Do not remove
  existing barriers.
- **W1C registers**: clear write-1-to-clear flags with a direct write
  (`REG = FLAG`), never read-modify-write (`REG |= FLAG`).
- **Ring buffers shared with ISRs**: indices are `volatile`; be explicit
  about read/write ordering between ISR and thread context.

## Sync tool (`tools/update_drivers.py`)

The sync tool lives in this repository and is copied into consuming projects
by `init`. It downloads a snapshot of this repo at the ref pinned in the
project's `drivers.lock`, copies the requested modules, updates the Keil
`.uvprojx`, and **self-updates** so the script always matches the pinned
drivers.

### When you modify the sync tool

- Follow the Python style guide: [lukutar-python-dev][skill-py] when
  available. If not, use the conventions already present in the file.
- Update the file header (`Date`, `Version`) after every change.
- The script has **zero third-party dependencies** (stdlib only). Keep it
  that way — it must run in arbitrary project environments with only
  Python 3.9+.
- Run a smoke test before committing: run `python tools/update_drivers.py
  --help` and `python tools/update_drivers.py sync --help` from this repo
  to verify import and parse correctness.

## Testing

There is no unit test framework. Validation is done by:

1. Compiling against at least one consuming project (Keil uVision,
   `build.bat Debug` in the project repo).
2. Running the consuming project's BITE/diagnostics on hardware when the
   change touches timing, interrupts, or EEPROM.

State clearly in your report what you verified and what you could not
verify (e.g. "compiled only, not tested on hardware").

## Commits and releases

- Commit messages: Conventional Commits, e.g.
  `feat(uart): add MIL_UART_GetTxFree`, `fix(eeprom): mask IRQs in EraseAll`.
- Do not commit unless the user explicitly asks.
- Repository tags (`vYYYY.MM.DD`) mark tested snapshots that projects pin
  in `drivers.lock`. Tagging is done by the maintainer, not by agents.

[skill-c]: C:\Users\oppp0\.zcode\skills\lukutar-c-style-guide\SKILL.md
[skill-py]: C:\Users\oppp0\.zcode\skills\lukutar-python-dev\SKILL.md

# update_drivers.py

Synchronize MIL driver modules from the [MIL Drivers][repo] repository into
a firmware project.

## Requirements

- Python 3.9+ (no third-party dependencies)
- Internet access (to download the repository snapshot)

## Quick start

### New project (bootstrap)

No need to clone the MIL_Drivers repo. One command from PowerShell:

```powershell
irm https://raw.githubusercontent.com/Mistress-Lukutar/MIL-Drivers/main/tools/update_drivers.py | python - init --repo Mistress-Lukutar/MIL-Drivers --ref v2026.07.30
```

Or on any shell with `curl`:

```bash
curl -sL https://raw.githubusercontent.com/Mistress-Lukutar/MIL-Drivers/main/tools/update_drivers.py | python - init --repo Mistress-Lukutar/MIL-Drivers --ref v2026.07.30
```

The script detects it was piped from stdin, saves itself to
`tools/update_drivers.py`, and re-executes from disk. After that,
`python tools/update_drivers.py sync` works as usual.

### Migrating an existing project

If your project has a flat `Driver/Inc/` and `Driver/Src/` layout with
old in-tree driver copies:

```bash
# Preview what would be done
python tools/update_drivers.py migrate --dry-run \
  --repo Mistress-Lukutar/MIL-Drivers --ref v2026.07.30

# Apply: removes old Driver/ dir, cleans Keil groups, runs init
python tools/update_drivers.py migrate \
  --repo Mistress-Lukutar/MIL-Drivers --ref v2026.07.30
```

### After first setup

```bash
# Sync to the pinned version
python tools/update_drivers.py sync

# Pin a newer version and re-sync
python tools/update_drivers.py sync --ref v2026.08.15

# CI: verify no local drift
python tools/update_drivers.py check
```

## Commands

### `init`

Create `drivers.lock` and perform the first module sync.

```bash
python tools/update_drivers.py init \
  --repo Mistress-Lukutar/MIL-Drivers \
  --ref main \
  --platform MDR1986BE9x \
  --dest Drivers \
  --modules MIL_Time,MIL_GPIO,MIL_Uart
```

| Option | Default | Description |
|--------|---------|-------------|
| `--repo` | (required) | Repo specifier: `owner/MIL_Drivers`, `github.com/owner/MIL_Drivers`, or full URL |
| `--ref` | `main` | Git ref to pin (tag, branch, or commit SHA) |
| `--platform` | `MDR1986BE9x` | Platform directory under `Drivers/` |
| `--dest` | `Drivers` | Target directory in the project |
| `--modules` | *(all)* | Comma-separated module list; omit to sync all available modules |
| `--force` | off | Overwrite locally modified files without prompting |
| `--refresh` | off | Re-download the snapshot even if cached |
| `--no-keil` | off | Skip `.uvprojx` modification |

### `sync`

Update driver files to match the pinned snapshot in `drivers.lock`.

```bash
python tools/update_drivers.py sync [--ref <new-ref>] [--force] [--refresh]
```

If the snapshot's copy of this script differs from the local one, the script
replaces itself and re-executes automatically.

| Option | Default | Description |
|--------|---------|-------------|
| `--ref` | *(from lock)* | Switch to a different ref before syncing |
| `--force` | off | Overwrite locally modified files |
| `--refresh` | off | Re-download the snapshot |
| `--no-keil` | off | Skip `.uvprojx` modification |
| `--no-self-update` | off | Do not replace this script from the snapshot |

### `check`

Verify that all driver files on disk match the hashes in `drivers.lock`.
Exits with code 0 on success, 1 on mismatch. Suitable for CI.

```bash
python tools/update_drivers.py check
```

### `list`

List all modules available in the snapshot. Pinned modules are marked.

```bash
python tools/update_drivers.py list [--ref <ref>]
```

### `migrate`

Migrate from an old flat `Driver/Inc/` + `Driver/Src/` layout to the new
per-module `Drivers/<Module>/` layout. Removes old Keil groups, cleans old
include paths, deletes the old `Driver/` directory, and runs `init` with
the given parameters.

```bash
python tools/update_drivers.py migrate \
  --repo Mistress-Lukutar/MIL-Drivers \
  --ref v2026.07.30 \
  [--dry-run] [--force]
```

| Option | Default | Description |
|--------|---------|-------------|
| `--repo` | (required) | Repo specifier |
| `--ref` | `main` | Git ref to pin |
| `--platform` | `MDR1986BE9x` | Platform directory |
| `--dest` | `Drivers` | New driver directory |
| `--modules` | *(all)* | Comma-separated module list |
| `--old-driver-dir` | `Driver` | Old flat driver directory to remove |
| `--old-groups` | `Driver/Inc,Driver/Src` | Old Keil group names to remove |
| `--dry-run` | off | Show what would be done without doing it |
| `--force` | off | Overwrite locally modified files |
| `--no-keil` | off | Skip `.uvprojx` modification |

## drivers.lock format

The lock file is a JSON file in the project root:

```json
{
  "version": 1,
  "repo": "github.com/Mistress-Lukutar/MIL-Drivers",
  "ref": "v2026.07.30",
  "platform": "MDR1986BE9x",
  "dest": "Drivers",
  "modules": [
    "MIL_Time",
    "MIL_GPIO",
    "MIL_Uart"
  ],
  "files": {
    "Drivers/MIL_Time/MIL_Time.c": "<sha256>",
    "Drivers/MIL_Time/MIL_Time.h": "<sha256>",
    "Drivers/MIL_GPIO/MIL_GPIO.c": "<sha256>",
    "Drivers/MIL_GPIO/MIL_GPIO.h": "<sha256>"
  },
  "script_sha256": "<sha256>",
  "synced_at": "2026-07-30T12:00:00"
}
```

| Field | Description |
|-------|-------------|
| `version` | Lock format schema version |
| `repo` | Normalized repo specifier (`owner/name`) |
| `ref` | Pinned git ref |
| `platform` | Platform directory in the MIL Drivers repo |
| `dest` | Driver directory relative to the project root |
| `modules` | Modules to sync |
| `files` | Map of project-relative paths to SHA-256 hashes |
| `script_sha256` | Hash of `tools/update_drivers.py` at sync time |
| `synced_at` | ISO 8601 timestamp of last sync |

## How it works

1. Reads `drivers.lock` to find the pinned repo, ref, and module list.
2. Downloads a `.tar.gz` snapshot from `codeload.github.com` and caches it
   in `.drivers-cache/` inside the project (gitignored).
3. Self-updates: replaces the project copy of this script with the one from
   the snapshot and re-executes.
4. Copies each module's `.c` and `.h` files into `<dest>/<module>/`.
5. Detects locally modified files and refuses to overwrite them unless
   `--force` is passed.
6. If a `.uvprojx` file is found, adds the driver files to a **Drivers**
   group in every build target and appends include paths.
7. Rewrites `drivers.lock` with updated hashes.

## Keil uVision integration

The script modifies `.uvprojx` files automatically:

- Creates a **Drivers** group in each build target if it does not exist.
- Adds each `.c` file (FileType 1) and `.h` file (FileType 5) to the group.
- Appends `.\Drivers\<Module>` to the C compiler include path of each target.
- Writes a `.bak` backup before saving.

The script does **not** remove old driver groups or files — that is a
one-time migration step performed manually when switching from in-tree
drivers.

[repo]: https://github.com/Mistress-Lukutar/MIL-Drivers

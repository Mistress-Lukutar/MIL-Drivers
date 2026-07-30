'''
File:   update_drivers.py
Brief:  Sync MIL driver modules from the MIL_Drivers repository into a project.
Author: Mistress-Lukutar
Date:   2026-07-30
Version: v1.1.0

The script downloads a snapshot of the MIL_Drivers GitHub repository at the
ref pinned in the project's `drivers.lock`, copies the requested modules into
the project's driver directory, registers them in a Keil uVision project
(.uvprojx) when one is present, and rewrites `drivers.lock` with the new
file hashes.

The canonical copy of this script lives in the MIL_Drivers repository under
`tools/update_drivers.py`. On every run the script replaces its own copy in
the project with the one from the downloaded snapshot and re-executes
itself, so the project always uses the script matching the pinned drivers.
'''

from __future__ import annotations

import argparse
import hashlib
import json
import logging
import os
import shutil
import sys
import tarfile
import tempfile
import urllib.error
import urllib.request
import xml.etree.ElementTree as ET
from datetime import datetime
from pathlib import Path

logger = logging.getLogger("update_drivers")

LOCK_FILENAME = "drivers.lock"  # pin file in the project root
CACHE_DIRNAME = ".drivers-cache"  # snapshot cache inside the project
SCRIPT_REL_PATH = Path("tools") / "update_drivers.py"  # path inside the repo
REEXEC_GUARD_ENV = "MIL_DRIVERS_REEXEC"  # prevents self-update exec loops
DEFAULT_PLATFORM = "MDR1986BE9x"  # default MCU family directory
DEFAULT_DEST = "Drivers"  # default driver directory in the project
KEIL_GROUP_NAME = "Drivers"  # group created in .uvprojx for driver files
KEIL_FILE_TYPE_C = "1"  # Keil FileType code for C sources
KEIL_FILE_TYPE_H = "5"  # Keil FileType code for headers
LOCK_FORMAT_VERSION = 1  # schema version of drivers.lock


class DriverSyncError(Exception):
    '''Base exception for all driver sync errors.'''


class LockNotFoundError(DriverSyncError):
    '''Raised when drivers.lock is missing in the project root.'''


class RemoteFetchError(DriverSyncError):
    '''Raised when the repository snapshot cannot be downloaded.'''


class LocalModificationError(DriverSyncError):
    '''Raised when locally modified driver files would be overwritten.'''


class KeilProjectError(DriverSyncError):
    '''Raised when the Keil project file cannot be updated.'''


def normalize_repo(repo: str) -> tuple[str, str]:
    '''Extract the GitHub owner and repository name from a repo specifier.

    Args:
        repo: Repo in any of the forms "owner/name",
            "github.com/owner/name", "https://github.com/owner/name(.git)".

    Returns:
        Tuple of (owner, name).

    Raises:
        ValueError: If the specifier cannot be parsed.
    '''
    text = repo.strip()
    for prefix in ("https://", "http://"):
        if text.startswith(prefix):
            text = text[len(prefix):]
    if text.startswith("github.com/"):
        text = text[len("github.com/"):]
    if text.endswith(".git"):
        text = text[: -len(".git")]
    parts = [p for p in text.split("/") if p]
    if len(parts) != 2:
        raise ValueError(
            f"Cannot parse repo specifier '{repo}', expected 'owner/name'",
        )
    return parts[0], parts[1]


def sha256_file(path: Path) -> str:
    '''Compute the SHA-256 hex digest of a file.

    Args:
        path: File to hash.

    Returns:
        Lowercase hex digest string.
    '''
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(65536), b""):
            digest.update(chunk)
    return digest.hexdigest()


def load_lock(project_root: Path) -> dict:
    '''Load and minimally validate drivers.lock.

    Args:
        project_root: Project directory containing the lock file.

    Returns:
        Parsed lock data.

    Raises:
        LockNotFoundError: If the lock file does not exist.
        DriverSyncError: If the lock file is malformed.
    '''
    lock_path = project_root / LOCK_FILENAME
    if not lock_path.is_file():
        raise LockNotFoundError(
            f"{LOCK_FILENAME} not found in {project_root}; "
            "run 'update_drivers.py init' first",
        )
    try:
        data = json.loads(lock_path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        raise DriverSyncError(f"Malformed {LOCK_FILENAME}: {exc}") from exc
    for key in ("repo", "ref", "platform", "dest", "modules"):
        if key not in data:
            raise DriverSyncError(f"{LOCK_FILENAME} is missing key '{key}'")
    return data


def save_lock(project_root: Path, data: dict) -> None:
    '''Write drivers.lock with stable formatting.

    Args:
        project_root: Project directory.
        data: Lock data to serialize.
    '''
    lock_path = project_root / LOCK_FILENAME
    lock_path.write_text(
        json.dumps(data, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )


def _safe_ref_name(ref: str) -> str:
    '''Turn a git ref into a filesystem-safe directory name.'''
    return "".join(ch if ch.isalnum() or ch in ".-_" else "_" for ch in ref)


def fetch_snapshot(
    owner: str,
    name: str,
    ref: str,
    cache_root: Path,
    refresh: bool = False,
) -> Path:
    '''Download and extract a repository snapshot tarball into the cache.

    Args:
        owner: GitHub repository owner.
        name: GitHub repository name.
        ref: Git ref (tag, branch, or commit SHA).
        cache_root: Cache directory (created if missing).
        refresh: Re-download even if the snapshot is already cached.

    Returns:
        Path to the extracted snapshot root.

    Raises:
        RemoteFetchError: If the download or extraction fails.
    '''
    target = cache_root / _safe_ref_name(ref)
    if target.is_dir() and not refresh:
        logger.info("Using cached snapshot %s", target)
        return target

    url = f"https://codeload.github.com/{owner}/{name}/tar.gz/{ref}"
    logger.info("Downloading %s", url)
    cache_root.mkdir(parents=True, exist_ok=True)

    with tempfile.NamedTemporaryFile(suffix=".tar.gz", delete=False) as tmp:
        tmp_path = Path(tmp.name)
        try:
            with urllib.request.urlopen(url, timeout=60) as response:
                shutil.copyfileobj(response, tmp)
        except (urllib.error.URLError, TimeoutError) as exc:
            tmp.close()
            tmp_path.unlink(missing_ok=True)
            raise RemoteFetchError(
                f"Failed to download snapshot '{ref}' of {owner}/{name}: {exc}",
            ) from exc

    staging = target.with_name(target.name + ".tmp")
    shutil.rmtree(staging, ignore_errors=True)
    staging.mkdir(parents=True)
    try:
        with tarfile.open(tmp_path, "r:gz") as archive:
            for member in archive.getmembers():
                # Strip the leading "<repo>-<ref>/" component and skip
                # anything that would escape the staging directory.
                parts = Path(member.name).parts[1:]
                if not parts or ".." in parts:
                    continue
                member.name = str(Path(*parts))
                archive.extract(member, staging, filter="data")
    except (tarfile.TarError, OSError) as exc:
        shutil.rmtree(staging, ignore_errors=True)
        raise RemoteFetchError(f"Failed to extract snapshot: {exc}") from exc
    finally:
        tmp_path.unlink(missing_ok=True)

    shutil.rmtree(target, ignore_errors=True)
    staging.rename(target)
    return target


def self_update(snapshot_root: Path, no_self_update: bool) -> None:
    '''Replace the running script with the snapshot copy and re-execute.

    The project copy always matches the pinned driver snapshot, so lock
    format changes and script fixes propagate together with the drivers.

    Args:
        snapshot_root: Extracted repository snapshot.
        no_self_update: Skip self-update (used for development).
    '''
    if no_self_update or os.environ.get(REEXEC_GUARD_ENV):
        return
    remote_script = snapshot_root / SCRIPT_REL_PATH
    local_script = Path(__file__).resolve()
    if not remote_script.is_file():
        logger.warning("Snapshot has no %s; self-update skipped", SCRIPT_REL_PATH)
        return
    if sha256_file(remote_script) == sha256_file(local_script):
        return
    logger.info("Self-updating %s from snapshot and re-running", local_script)
    shutil.copy2(remote_script, local_script)
    os.environ[REEXEC_GUARD_ENV] = "1"
    os.execv(sys.executable, [sys.executable, str(local_script), *sys.argv[1:]])


def _collect_module_files(module_dir: Path) -> list[Path]:
    '''List the files belonging to a driver module.'''
    return sorted(p for p in module_dir.iterdir() if p.is_file())


def _find_local_modifications(
    project_root: Path,
    lock: dict,
    new_hashes: dict[str, str],
) -> list[str]:
    '''Find driver files edited locally since the last sync.

    Args:
        project_root: Project directory.
        lock: Previous lock data.
        new_hashes: Hashes of the files about to be written.

    Returns:
        Relative paths of files that were modified locally and would be
        overwritten with different content.
    '''
    modified = []
    for rel_path, old_hash in lock.get("files", {}).items():
        disk_path = project_root / rel_path
        if not disk_path.is_file():
            continue
        disk_hash = sha256_file(disk_path)
        if disk_hash != old_hash and disk_hash != new_hashes.get(rel_path):
            modified.append(rel_path)
    return modified


def sync_modules(
    project_root: Path,
    snapshot_root: Path,
    lock: dict,
    force: bool,
) -> dict[str, str]:
    '''Copy the pinned modules from the snapshot into the project.

    Args:
        project_root: Project directory.
        snapshot_root: Extracted repository snapshot.
        lock: Lock data (repo/ref/platform/dest/modules).
        force: Overwrite locally modified files.

    Returns:
        Mapping of project-relative file paths to their SHA-256 hashes.

    Raises:
        DriverSyncError: If a module is missing in the snapshot.
        LocalModificationError: If local edits would be overwritten.
    '''
    platform = lock["platform"]
    dest = lock["dest"]
    new_hashes: dict[str, str] = {}
    copy_plan: list[tuple[Path, Path]] = []

    for module in lock["modules"]:
        module_src = snapshot_root / "Drivers" / platform / module
        if not module_src.is_dir():
            available = sorted(
                p.name for p in (snapshot_root / "Drivers" / platform).iterdir()
            )
            raise DriverSyncError(
                f"Module '{module}' not found in snapshot "
                f"(available: {', '.join(available)})",
            )
        for src_file in _collect_module_files(module_src):
            rel_path = Path(dest) / module / src_file.name
            copy_plan.append((src_file, project_root / rel_path))
            new_hashes[rel_path.as_posix()] = sha256_file(src_file)

    modified = _find_local_modifications(project_root, lock, new_hashes)
    if modified and not force:
        raise LocalModificationError(
            "Locally modified driver files would be overwritten:\n  "
            + "\n  ".join(modified)
            + "\nCommit or revert these changes, or re-run with --force.",
        )

    for src_file, dst_file in copy_plan:
        dst_file.parent.mkdir(parents=True, exist_ok=True)
        if dst_file.is_file() and sha256_file(dst_file) == sha256_file(src_file):
            continue
        shutil.copy2(src_file, dst_file)
        logger.info(
            "Copied %s", dst_file.relative_to(project_root).as_posix(),
        )
    return new_hashes


def _keil_rel_path(dest: str, module: str, filename: str) -> str:
    '''Build a Keil-style relative path (.\\Drivers\\MIL_X\\file.c).'''
    return ".\\" + "\\".join([dest, module, filename])


def update_keil_project(
    uvprojx_path: Path,
    dest: str,
    modules: list[str],
) -> bool:
    '''Register driver files and include paths in a Keil uVision project.

    Adds each module's .c/.h files to a group named after KEIL_GROUP_NAME
    in every target, and appends each module directory to the C compiler
    include path of every target. A .bak backup is written before saving.

    Args:
        uvprojx_path: Path to the .uvprojx file.
        dest: Driver directory relative to the project root.
        modules: Modules to register.

    Returns:
        True if the project file was modified.

    Raises:
        KeilProjectError: If the project file cannot be parsed or written.
    '''
    try:
        tree = ET.parse(uvprojx_path)
    except ET.ParseError as exc:
        raise KeilProjectError(f"Cannot parse {uvprojx_path}: {exc}") from exc
    root = tree.getroot()
    modified = False

    for target in root.iter("Target"):
        target_name = target.findtext("TargetName", default="?")
        groups = target.find("Groups")
        if groups is None:
            continue

        existing_paths = {
            f.findtext("FilePath", default="")
            for f in target.iter("File")
        }
        group = None
        for candidate in groups.findall("Group"):
            if candidate.findtext("GroupName") == KEIL_GROUP_NAME:
                group = candidate
                break
        if group is None:
            group = ET.SubElement(groups, "Group")
            ET.SubElement(group, "GroupName").text = KEIL_GROUP_NAME
            ET.SubElement(group, "Files")
        files_elem = group.find("Files")
        if files_elem is None:
            files_elem = ET.SubElement(group, "Files")

        for module in modules:
            module_dir = uvprojx_path.parent / dest / module
            for file_path in sorted(module_dir.iterdir()):
                rel = _keil_rel_path(dest, module, file_path.name)
                if rel in existing_paths or file_path.suffix not in (".c", ".h"):
                    continue
                file_elem = ET.SubElement(files_elem, "File")
                ET.SubElement(file_elem, "FileName").text = file_path.name
                ET.SubElement(file_elem, "FileType").text = (
                    KEIL_FILE_TYPE_C
                    if file_path.suffix == ".c"
                    else KEIL_FILE_TYPE_H
                )
                ET.SubElement(file_elem, "FilePath").text = rel
                modified = True
                logger.info(
                    "Added %s to target '%s' group '%s'",
                    rel,
                    target_name,
                    KEIL_GROUP_NAME,
                )

        include_elem = target.find(
            "./TargetOption/TargetArmAds/Cads/VariousControls/IncludePath",
        )
        if include_elem is not None:
            current = include_elem.text or ""
            entries = [e for e in current.split(";") if e]
            for module in modules:
                entry = f".\\{dest}\\{module}"
                if entry not in entries:
                    entries.append(entry)
                    modified = True
                    logger.info(
                        "Added include path %s to target '%s'",
                        entry,
                        target_name,
                    )
            include_elem.text = ";".join(entries)

    if not modified:
        logger.info("Keil project %s is already up to date", uvprojx_path.name)
        return False

    backup = uvprojx_path.with_suffix(uvprojx_path.suffix + ".bak")
    shutil.copy2(uvprojx_path, backup)
    logger.info("Backup written to %s", backup.name)

    ET.indent(tree, space="  ")
    try:
        tree.write(uvprojx_path, encoding="utf-8", xml_declaration=True)
    except OSError as exc:
        raise KeilProjectError(f"Cannot write {uvprojx_path}: {exc}") from exc
    return True


def find_keil_projects(project_root: Path) -> list[Path]:
    '''Find Keil uVision project files in the project root.'''
    return sorted(project_root.glob("*.uvprojx"))


def _remove_keil_groups(
    uvprojx_path: Path,
    group_names: list[str],
) -> bool:
    '''Remove named groups from every target in a Keil project.

    Args:
        uvprojx_path: Path to the .uvprojx file.
        group_names: Group names to remove (e.g. ["Driver/Inc", "Driver/Src"]).

    Returns:
        True if any group was removed.
    '''
    try:
        tree = ET.parse(uvprojx_path)
    except ET.ParseError as exc:
        raise KeilProjectError(f"Cannot parse {uvprojx_path}: {exc}") from exc
    root = tree.getroot()
    modified = False

    for target in root.iter("Target"):
        groups = target.find("Groups")
        if groups is None:
            continue
        for group in list(groups.findall("Group")):
            if group.findtext("GroupName") in group_names:
                groups.remove(group)
                modified = True
                logger.info(
                    "Removed group '%s' from target '%s'",
                    group.findtext("GroupName"),
                    target.findtext("TargetName", default="?"),
                )

    if modified:
        backup = uvprojx_path.with_suffix(uvprojx_path.suffix + ".bak")
        shutil.copy2(uvprojx_path, backup)
        ET.indent(tree, space="  ")
        tree.write(uvprojx_path, encoding="utf-8", xml_declaration=True)
    return modified


def _remove_include_entries(
    uvprojx_path: Path,
    prefixes: list[str],
) -> bool:
    '''Remove include path entries matching given prefixes from every target.

    Args:
        uvprojx_path: Path to the .uvprojx file.
        prefixes: Path prefixes to remove (e.g. [".\\Driver\\Inc"]).

    Returns:
        True if any entry was removed.
    '''
    try:
        tree = ET.parse(uvprojx_path)
    except ET.ParseError as exc:
        raise KeilProjectError(f"Cannot parse {uvprojx_path}: {exc}") from exc
    root = tree.getroot()
    modified = False

    for target in root.iter("Target"):
        include_elem = target.find(
            "./TargetOption/TargetArmAds/Cads/VariousControls/IncludePath",
        )
        if include_elem is None or not include_elem.text:
            continue
        entries = [e for e in include_elem.text.split(";") if e]
        new_entries = [
            e for e in entries
            if not any(e.lower().startswith(p.lower()) for p in prefixes)
        ]
        if len(new_entries) != len(entries):
            modified = True
            logger.info(
                "Cleaned include paths in target '%s'",
                target.findtext("TargetName", default="?"),
            )
            include_elem.text = ";".join(new_entries)

    if modified:
        ET.indent(tree, space="  ")
        tree.write(uvprojx_path, encoding="utf-8", xml_declaration=True)
    return modified


def cmd_migrate(args: argparse.Namespace) -> int:
    '''Migrate from flat Driver/ layout to per-module Drivers/ layout.

    Detects the old Keil groups ("Driver/Inc", "Driver/Src"), removes them
    from every .uvprojx target, deletes the old Driver/ directory, and
    delegates to `cmd_init` with the given parameters.

    Args:
        args: Parsed CLI arguments (same flags as init, plus
            --old-driver-dir, --old-groups).

    Returns:
        Exit code (0 on success).
    '''
    project_root = Path(args.project).resolve()
    old_dir = project_root / args.old_driver_dir
    if not old_dir.is_dir():
        logger.info(
            "Old driver directory '%s' not found, running init directly",
            args.old_driver_dir,
        )
        return cmd_init(args)

    if args.dry_run:
        removed_files = sorted(p for p in old_dir.rglob("*") if p.is_file())
        for f in removed_files:
            print(f"  would delete: {f.relative_to(project_root).as_posix()}")
        for uvprojx in find_keil_projects(project_root):
            print(f"  would clean groups in {uvprojx.name}")
        print("Dry run complete. Re-run without --dry-run to apply.")
        return 0

    old_groups = [g.strip() for g in args.old_groups.split(",") if g.strip()]
    old_include_prefixes = [
        f".\\{args.old_driver_dir}\\Inc",
        f".\\{args.old_driver_dir}\\Src",
    ]

    # Clean Keil projects
    if not args.no_keil:
        for uvprojx in find_keil_projects(project_root):
            _remove_keil_groups(uvprojx, old_groups)
            _remove_include_entries(uvprojx, old_include_prefixes)

    # Remove old driver directory
    shutil.rmtree(old_dir, ignore_errors=True)
    logger.info("Removed old driver directory '%s'", args.old_driver_dir)

    # Run init to set up new layout
    return cmd_init(args)


def cmd_init(args: argparse.Namespace) -> int:
    '''Create drivers.lock and perform the first sync.'''
    project_root = Path(args.project).resolve()
    owner, name = normalize_repo(args.repo)
    snapshot = fetch_snapshot(
        owner,
        name,
        args.ref,
        project_root / CACHE_DIRNAME,
        refresh=args.refresh,
    )
    platform_dir = snapshot / "Drivers" / args.platform
    if not platform_dir.is_dir():
        raise DriverSyncError(
            f"Platform '{args.platform}' not found in the snapshot",
        )
    if args.modules:
        modules = [m.strip() for m in args.modules.split(",") if m.strip()]
    else:
        modules = sorted(p.name for p in platform_dir.iterdir() if p.is_dir())
        logger.info("Selected all modules: %s", ", ".join(modules))

    lock = {
        "version": LOCK_FORMAT_VERSION,
        "repo": f"github.com/{owner}/{name}",
        "ref": args.ref,
        "platform": args.platform,
        "dest": args.dest,
        "modules": modules,
        "files": {},
    }
    lock["files"] = sync_modules(project_root, snapshot, lock, force=args.force)
    lock["script_sha256"] = sha256_file(snapshot / SCRIPT_REL_PATH)
    lock["synced_at"] = datetime.now().isoformat(timespec="seconds")
    save_lock(project_root, lock)

    if not args.no_keil:
        for uvprojx in find_keil_projects(project_root):
            update_keil_project(uvprojx, lock["dest"], modules)

    logger.info("Initialized %s (ref %s, %d modules)",
                LOCK_FILENAME, args.ref, len(modules))
    return 0


def cmd_sync(args: argparse.Namespace) -> int:
    '''Synchronize drivers with the pinned snapshot.'''
    project_root = Path(args.project).resolve()
    lock = load_lock(project_root)
    if args.ref:
        lock["ref"] = args.ref
    owner, name = normalize_repo(lock["repo"])
    snapshot = fetch_snapshot(
        owner,
        name,
        lock["ref"],
        project_root / CACHE_DIRNAME,
        refresh=args.refresh,
    )
    self_update(snapshot, no_self_update=args.no_self_update)

    lock["files"] = sync_modules(project_root, snapshot, lock, force=args.force)
    lock["script_sha256"] = sha256_file(snapshot / SCRIPT_REL_PATH)
    lock["synced_at"] = datetime.now().isoformat(timespec="seconds")
    save_lock(project_root, lock)

    if not args.no_keil:
        for uvprojx in find_keil_projects(project_root):
            update_keil_project(uvprojx, lock["dest"], lock["modules"])

    logger.info("Sync complete (ref %s)", lock["ref"])
    return 0


def cmd_check(args: argparse.Namespace) -> int:
    '''Verify that the driver files match drivers.lock (for CI).'''
    project_root = Path(args.project).resolve()
    lock = load_lock(project_root)
    problems = []
    for rel_path, expected_hash in lock.get("files", {}).items():
        disk_path = project_root / rel_path
        if not disk_path.is_file():
            problems.append(f"missing: {rel_path}")
        elif sha256_file(disk_path) != expected_hash:
            problems.append(f"modified: {rel_path}")
    if problems:
        for problem in problems:
            logger.error("%s", problem)
        return 1
    logger.info("All %d driver files match %s",
                len(lock.get("files", {})), LOCK_FILENAME)
    return 0


def cmd_list(args: argparse.Namespace) -> int:
    '''List modules available in the pinned (or given) snapshot.'''
    project_root = Path(args.project).resolve()
    lock = load_lock(project_root)
    ref = args.ref or lock["ref"]
    owner, name = normalize_repo(lock["repo"])
    snapshot = fetch_snapshot(
        owner,
        name,
        ref,
        project_root / CACHE_DIRNAME,
        refresh=args.refresh,
    )
    platform_dir = snapshot / "Drivers" / lock["platform"]
    if not platform_dir.is_dir():
        raise DriverSyncError(
            f"Platform '{lock['platform']}' not found in the snapshot",
        )
    for module_dir in sorted(platform_dir.iterdir()):
        if module_dir.is_dir():
            pinned = " (pinned)" if module_dir.name in lock["modules"] else ""
            print(f"{module_dir.name}{pinned}")
    return 0


def build_parser() -> argparse.ArgumentParser:
    '''Build the CLI argument parser.'''
    # Shared parent so --project and -v work in any position
    parent = argparse.ArgumentParser(add_help=False)
    parent.add_argument(
        "--project",
        default=".",
        help="Project root containing drivers.lock (default: cwd)",
    )
    parent.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        help="Enable debug logging",
    )

    parser = argparse.ArgumentParser(
        description="Sync MIL drivers from the MIL_Drivers repository.",
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    init = subparsers.add_parser(
        "init",
        help="Create drivers.lock and perform the first sync",
        parents=[parent],
    )
    init.add_argument("--repo", required=True,
                      help="Repo specifier, e.g. owner/MIL_Drivers")
    init.add_argument("--ref", default="main", help="Git ref to pin")
    init.add_argument("--platform", default=DEFAULT_PLATFORM)
    init.add_argument("--dest", default=DEFAULT_DEST,
                      help="Driver directory in the project")
    init.add_argument("--modules", default="",
                      help="Comma-separated module list (default: all)")
    init.add_argument("--force", action="store_true")
    init.add_argument("--refresh", action="store_true",
                      help="Re-download the snapshot")
    init.add_argument("--no-keil", action="store_true",
                      help="Do not touch .uvprojx files")
    init.set_defaults(func=cmd_init)

    sync = subparsers.add_parser(
        "sync",
        help="Sync drivers with the pinned snapshot",
        parents=[parent],
    )
    sync.add_argument("--ref", default="",
                      help="Switch the pinned ref before syncing")
    sync.add_argument("--force", action="store_true",
                      help="Overwrite locally modified driver files")
    sync.add_argument("--refresh", action="store_true",
                      help="Re-download the snapshot")
    sync.add_argument("--no-keil", action="store_true",
                      help="Do not touch .uvprojx files")
    sync.add_argument("--no-self-update", action="store_true",
                      help="Do not replace this script from the snapshot")
    sync.set_defaults(func=cmd_sync)

    check = subparsers.add_parser(
        "check",
        help="Verify driver files against drivers.lock",
        parents=[parent],
    )
    check.set_defaults(func=cmd_check)

    list_cmd = subparsers.add_parser(
        "list",
        help="List modules available in the snapshot",
        parents=[parent],
    )
    list_cmd.add_argument("--ref", default="",
                          help="Ref to list instead of the pinned one")
    list_cmd.add_argument("--refresh", action="store_true")
    list_cmd.set_defaults(func=cmd_list)

    migrate = subparsers.add_parser(
        "migrate",
        help="Migrate from flat Driver/ to per-module Drivers/ layout",
        parents=[parent],
    )
    migrate.add_argument("--repo", required=True,
                         help="Repo specifier, e.g. owner/MIL_Drivers")
    migrate.add_argument("--ref", default="main", help="Git ref to pin")
    migrate.add_argument("--platform", default=DEFAULT_PLATFORM)
    migrate.add_argument("--dest", default=DEFAULT_DEST,
                         help="New driver directory in the project")
    migrate.add_argument("--modules", default="",
                         help="Comma-separated module list (default: all)")
    migrate.add_argument("--old-driver-dir", default="Driver",
                         help="Old flat driver directory to remove")
    migrate.add_argument("--old-groups", default="Driver/Inc,Driver/Src",
                         help="Old Keil group names to remove (comma-sep)")
    migrate.add_argument("--force", action="store_true")
    migrate.add_argument("--dry-run", action="store_true",
                         help="Show what would be done without doing it")
    migrate.add_argument("--refresh", action="store_true")
    migrate.add_argument("--no-keil", action="store_true")
    migrate.set_defaults(func=cmd_migrate)

    return parser


def _is_piped_or_stdin() -> bool:
    '''Detect if the script was executed via pipe or with stdin source.'''
    if not hasattr(sys, "frozen") and getattr(sys, "argv", [""])[0] in (
        "-",
        "<stdin>",
    ):
        return True
    # Python may set __file__ to "<stdin>" when reading from pipe
    try:
        return Path(__file__).name == "<stdin>"
    except (TypeError, ValueError):
        return True


def _bootstrap_and_reexec(argv: list[str] | None = None) -> None:
    '''Bootstrap: save the piped script to tools/update_drivers.py and re-exec.

    When the script is run via a one-liner (e.g. ``irm ... | python -``),
    it has no file on disk and ``__file__`` is ``<stdin>``. This function
    saves the in-memory source to the project's ``tools/update_drivers.py``,
    then re-executes from that path with the same arguments so the script
    can find itself for self-update later.

    Args:
        argv: Command-line arguments (defaults to ``sys.argv``).
    '''
    argv = argv or sys.argv
    project_root = Path.cwd()

    # Determine the target script path
    # Allow --project to override cwd for the target path resolution
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument("--project", default=".")
    parser.add_argument("command", nargs="?", default=None)
    parsed, _ = parser.parse_known_args(argv[1:])
    project_root = Path(parsed.project).resolve()

    script_dest = project_root / "tools" / "update_drivers.py"
    script_dest.parent.mkdir(parents=True, exist_ok=True)

    # Read the piped source from stdin and write it
    source = sys.stdin.read()
    script_dest.write_text(source, encoding="utf-8")
    logger.info("Bootstrapped %s", script_dest)

    # Re-exec from the saved file
    os.execv(sys.executable, [sys.executable, str(script_dest), *argv[1:]])


def main() -> int:
    '''CLI entry point.'''
    argv = sys.argv

    # Bootstrap: if running from pipe/stdin, save to disk and re-exec
    if _is_piped_or_stdin():
        _bootstrap_and_reexec(argv)

    parser = build_parser()
    args = parser.parse_args()
    logging.basicConfig(
        level=logging.DEBUG if args.verbose else logging.INFO,
        format="%(levelname)s: %(message)s",
    )
    try:
        return args.func(args)
    except DriverSyncError as exc:
        logger.error("%s", exc)
        return 1


if __name__ == "__main__":
    sys.exit(main())

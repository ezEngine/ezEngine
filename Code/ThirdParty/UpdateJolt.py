#!/usr/bin/env python3
"""
Update the Jolt Physics library from GitHub.

Clones JoltPhysics into a temp folder, copies the Jolt/ subfolder into
Code/ThirdParty/Jolt/, and updates cgmanifest.json with the commit hash.
The temp clone is deleted automatically when the script exits.

Usage:
  python UpdateJolt.py                      # latest commit on default branch
  python UpdateJolt.py --commit <hash|tag>  # specific commit or tag
"""

import argparse
import json
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

REPO_URL = "https://github.com/jrouwe/JoltPhysics"
SCRIPT_DIR = Path(__file__).parent.resolve()
JOLT_DIR = SCRIPT_DIR / "Jolt"
PROTECTED_FILES = {"cgmanifest.json", "CMakeLists.txt", "init.cmake"}


def run(cmd, cwd=None):
    result = subprocess.run(cmd, cwd=cwd, check=True, capture_output=True, text=True)
    return result.stdout.strip()


def clone_latest(tmp: Path) -> Path:
    print(f"Cloning {REPO_URL} (latest)...")
    run(["git", "clone", "--depth=1", REPO_URL, "JoltPhysics"], cwd=tmp)
    return tmp / "JoltPhysics"


def clone_specific(tmp: Path, commit: str) -> Path:
    """Fetch a specific commit or tag without downloading the full history."""
    repo = tmp / "JoltPhysics"
    repo.mkdir()
    run(["git", "init"], cwd=repo)
    run(["git", "remote", "add", "origin", REPO_URL], cwd=repo)

    # Try as a branch/tag first (shallow), then fall back to direct commit fetch.
    try:
        print(f"Fetching '{commit}' from {REPO_URL}...")
        run(["git", "fetch", "--depth=1", "origin", commit], cwd=repo)
        run(["git", "checkout", "FETCH_HEAD"], cwd=repo)
    except subprocess.CalledProcessError:
        # Direct commit hash fetch (GitHub supports this).
        print(f"Branch/tag fetch failed, trying commit hash fetch...")
        run(["git", "fetch", "origin", commit], cwd=repo)
        run(["git", "checkout", "FETCH_HEAD"], cwd=repo)

    return repo


def clean_jolt_dir():
    print("Cleaning destination directory...")
    for item in JOLT_DIR.iterdir():
        if item.name in PROTECTED_FILES:
            continue
        if item.is_dir():
            shutil.rmtree(item)
        else:
            item.unlink()


def copy_jolt_files(src: Path):
    print(f"Copying files from {src} to {JOLT_DIR}...")
    for item in src.iterdir():
        dst = JOLT_DIR / item.name
        if item.is_dir():
            shutil.copytree(item, dst)
        else:
            shutil.copy2(item, dst)


def update_cgmanifest(commit_hash: str):
    manifest_path = JOLT_DIR / "cgmanifest.json"
    with open(manifest_path, "r", encoding="utf-8") as f:
        manifest = json.load(f)

    manifest["Registrations"][0]["component"]["git"]["commitHash"] = commit_hash

    with open(manifest_path, "w", encoding="utf-8", newline="\n") as f:
        json.dump(manifest, f, indent=2)
        f.write("\n")

    print(f"Updated cgmanifest.json with commit {commit_hash}")


def main():
    parser = argparse.ArgumentParser(description="Update Jolt Physics from GitHub.")
    parser.add_argument(
        "--commit",
        metavar="HASH_OR_TAG",
        help="Specific commit hash or tag to use (default: latest on default branch)",
    )
    args = parser.parse_args()

    with tempfile.TemporaryDirectory(prefix="JoltPhysics_") as tmpdir:
        tmp = Path(tmpdir)

        if args.commit:
            repo = clone_specific(tmp, args.commit)
        else:
            repo = clone_latest(tmp)

        commit_hash = run(["git", "rev-parse", "HEAD"], cwd=repo)
        print(f"Commit hash: {commit_hash}")

        src = repo / "Jolt"
        if not src.is_dir():
            print("Error: 'Jolt' subfolder not found in cloned repository.", file=sys.stderr)
            sys.exit(1)

        clean_jolt_dir()
        copy_jolt_files(src)
        update_cgmanifest(commit_hash)

        print(f"\nDone. Jolt updated to {commit_hash}")
        # TemporaryDirectory context manager deletes the clone here.


if __name__ == "__main__":
    main()

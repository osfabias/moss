#!/usr/bin/env uv run
# /// script
# requires-python = ">=3.8"
# ///
"""
Script to analyze malloc calls in the current commit.
Prints all malloc/calloc/realloc usages with file:line and original line text.

Usage with uv:
    uv run analyze_malloc.py

Usage with standard Python:
    python3 analyze_malloc.py
"""

import subprocess
import re

# Pattern used for git grep (POSIX ERE, no \s there).
# [[:space:]] works for spaces/tabs etc.
GIT_GREP_PATTERN = r"(malloc|calloc|realloc)[[:space:]]*\("


def get_malloc_usages(commit_hash="HEAD", source_dirs=None):
    """
    Get all malloc/calloc/realloc calls in a specific commit using git grep.

    Returns list of tuples: (file_path, line_number, line_text)
    """
    if source_dirs is None:
        source_dirs = ["src", "include"]

    # git grep command:
    #   git grep -n -E "<pattern>" <commit> -- <paths...>
    #   -n shows line numbers
    cmd = [
        "git",
        "grep",
        "-n",  # show line numbers
        "-E",
        GIT_GREP_PATTERN,
        commit_hash,
        "--",
        *source_dirs,
    ]

    result = subprocess.run(
        cmd,
        capture_output=True,
        text=True,
    )

    # git grep exit codes:
    #   0 = matches found
    #   1 = no matches
    #   >1 = real error
    if result.returncode not in (0, 1):
        err = result.stderr.strip()
        if err:
            print(f"[error] git grep failed: {err}", file=subprocess.stderr)
        return []

    if not result.stdout:
        return []

    usages = []
    for line in result.stdout.strip().split("\n"):
        if not line:
            continue
        # Format: file:line:line_text
        # Split on first two colons
        parts = line.split(":", 2)
        if len(parts) >= 3:
            file_path = parts[0]
            line_number = parts[1]
            line_text = parts[2]
            usages.append((file_path, line_number, line_text))

    return usages


def main():
    """Main function to print malloc usages in current commit."""
    print("Analyzing malloc calls in current commit...")

    usages = get_malloc_usages("HEAD")

    if not usages:
        print("No malloc/calloc/realloc calls found.")
        return

    print(f"\nFound {len(usages)} malloc/calloc/realloc call(s):\n")

    for file_path, line_number, line_text in usages:
        print(f"{file_path}:{line_number} | {line_text}")


if __name__ == "__main__":
    main()

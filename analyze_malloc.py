#!/usr/bin/env uv run
# /// script
# requires-python = ">=3.8"
# dependencies = [
#     "matplotlib>=3.5.0",
# ]
# ///
"""
Script to analyze malloc calls per commit in git history.
Shows a graph with commit time on horizontal axis and malloc call count on vertical axis.
Can also print all malloc usages in the current commit.

Usage with uv:
    uv run analyze_malloc.py [--current]

Usage with standard Python:
    python3 analyze_malloc.py [--current]
    (requires: pip install matplotlib)

Options:
    --current    Print all malloc/calloc/realloc usages in current commit instead of plotting history
"""

import subprocess
import re
import sys
from datetime import datetime
import matplotlib.pyplot as plt
import matplotlib.dates as mdates


# Python regex used to COUNT matches in text from git grep.
# This supports word boundaries and \s.
MALLOC_REGEX = re.compile(r"\b(malloc|calloc|realloc)\s*\(")

# Pattern used for git grep (POSIX ERE, no \s there).
# [[:space:]] works for spaces/tabs etc.
GIT_GREP_PATTERN = r"(malloc|calloc|realloc)[[:space:]]*\("


def get_git_commits():
    """Get list of commits with their commit times and hashes."""
    # %ct = committer date, UNIX timestamp
    result = subprocess.run(
        ["git", "log", "--pretty=format:%H|%ct", "--reverse"],
        capture_output=True,
        text=True,
        check=True,
    )

    commits = []
    for line in result.stdout.strip().split("\n"):
        if not line:
            continue
        parts = line.split("|", 1)
        if len(parts) != 2:
            continue

        commit_hash, ts_str = parts
        try:
            timestamp = int(ts_str.strip())
            date = datetime.fromtimestamp(timestamp)
        except ValueError:
            # Ignore malformed lines
            continue

        commits.append((commit_hash, date))

    return commits


def count_malloc_in_commit(commit_hash, source_dirs=None):
    """
    Count malloc/calloc/realloc calls in a specific commit using git grep.

    We use git grep to quickly retrieve candidate lines, then apply a Python
    regex with word boundaries to count the actual calls.

    This correctly counts:
        malloc(sizeof(int));
        malloc (sizeof(int));
        calloc (10, sizeof(int));
        realloc (ptr, new_size);
    """
    if source_dirs is None:
        source_dirs = ["src", "include"]

    # git grep command:
    #   git grep -h -E "<pattern>" <commit> -- <paths...>
    cmd = [
        "git",
        "grep",
        "-h",  # suppress filenames, only lines
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
            print(f"[warn] git grep failed for {commit_hash}: {err}")
        return 0

    if not result.stdout:
        return 0

    return len(MALLOC_REGEX.findall(result.stdout))


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


def print_current_malloc_usages():
    """Print all malloc usages in current commit."""
    print("Analyzing malloc calls in current commit...")

    usages = get_malloc_usages("HEAD")

    if not usages:
        print("No malloc/calloc/realloc calls found.")
        return

    print(f"\nFound {len(usages)} malloc/calloc/realloc call(s):\n")

    for file_path, line_number, line_text in usages:
        print(f"{file_path}:{line_number} | {line_text}")


def plot_malloc_history():
    """Main function to analyze and plot malloc calls."""
    print("Analyzing git history for malloc calls...")

    commits = get_git_commits()
    print(f"Found {len(commits)} commits")

    if not commits:
        print("No commits found!")
        return

    dates = []
    counts = []

    print("Counting malloc calls per commit...")
    for i, (commit_hash, date) in enumerate(commits):
        count = count_malloc_in_commit(commit_hash)
        dates.append(date)
        counts.append(count)

        if (i + 1) % 10 == 0 or i == len(commits) - 1:
            print(f"Processed {i + 1}/{len(commits)} commits...")

    print("Creating plot...")
    fig, ax = plt.subplots(figsize=(12, 6))

    ax.plot(dates, counts, marker="o", markersize=3, linewidth=1, alpha=0.7)
    ax.set_xlabel("Commit time", fontsize=12)
    ax.set_ylabel("Number of malloc/calloc/realloc calls", fontsize=12)
    ax.set_title("Malloc Calls per Commit Over Time", fontsize=14, fontweight="bold")
    ax.grid(True, alpha=0.3)

    ax.xaxis.set_major_formatter(mdates.DateFormatter("%Y-%m-%d\n%H:%M"))
    ax.xaxis.set_major_locator(mdates.AutoDateLocator())
    plt.xticks(rotation=0, ha="center")
    plt.tight_layout()

    print("\nStatistics:")
    print(f"  Total commits analyzed: {len(commits)}")
    print(f"  Total malloc calls (latest): {counts[-1] if counts else 0}")
    print(f"  Maximum malloc calls: {max(counts) if counts else 0}")
    print(f"  Minimum malloc calls: {min(counts) if counts else 0}")
    print(f"  Average malloc calls: {sum(counts) / len(counts) if counts else 0:.2f}")

    plt.show()


def main():
    """Main entry point."""
    if "--current" in sys.argv:
        print_current_malloc_usages()
    else:
        plot_malloc_history()


if __name__ == "__main__":
    main()

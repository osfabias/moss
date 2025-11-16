#!/usr/bin/env uv run
# /// script
# requires-python = ">=3.8"
# dependencies = [
#     "matplotlib>=3.5.0",
# ]
# ///
"""
Script to analyze malloc calls per commit in git history.
Shows a graph with days on horizontal axis and malloc call count on vertical axis.

Usage with uv:
    uv run analyze_malloc.py

Usage with standard Python:
    python3 analyze_malloc.py
    (requires: pip install matplotlib)
"""

import subprocess
import re
from datetime import datetime
import matplotlib.pyplot as plt
import matplotlib.dates as mdates


def get_git_commits():
    """Get list of commits with their dates and hashes."""
    result = subprocess.run(
        ["git", "log", "--pretty=format:%H|%ai", "--reverse"],
        capture_output=True,
        text=True,
        check=True,
    )

    commits = []
    for line in result.stdout.strip().split("\n"):
        if not line:
            continue
        parts = line.split("|")
        if len(parts) == 2:
            commit_hash, date_str = parts
            # Parse date string (format: 2025-01-15 10:30:45 +0000)
            date = datetime.strptime(date_str.split(" ")[0], "%Y-%m-%d")
            commits.append((commit_hash, date))

    return commits


def count_malloc_in_commit(commit_hash, source_dirs=None):
    """Count malloc calls in source files for a specific commit."""
    if source_dirs is None:
        source_dirs = ["src", "include"]

    total_count = 0

    # Get list of C/C++ source files in the commit
    try:
        result = subprocess.run(
            ["git", "ls-tree", "-r", "--name-only", commit_hash],
            capture_output=True,
            text=True,
            check=True,
        )

        files = result.stdout.strip().split("\n")

        for file_path in files:
            # Only check C/C++ source files in specified directories
            if not any(file_path.startswith(d) for d in source_dirs):
                continue

            if not (
                file_path.endswith(".c")
                or file_path.endswith(".cpp")
                or file_path.endswith(".h")
                or file_path.endswith(".hpp")
            ):
                continue

            # Get file content at this commit
            try:
                file_result = subprocess.run(
                    ["git", "show", f"{commit_hash}:{file_path}"],
                    capture_output=True,
                    text=True,
                    check=True,
                )

                content = file_result.stdout

                # Count malloc calls (including malloc, calloc, realloc)
                # Pattern matches: malloc(, calloc(, realloc(
                malloc_pattern = r"\b(malloc|calloc|realloc)\s*\("
                matches = re.findall(malloc_pattern, content)
                total_count += len(matches)

            except subprocess.CalledProcessError:
                # File might not exist in this commit, skip it
                continue

    except subprocess.CalledProcessError:
        # Commit might not have any files, return 0
        pass

    return total_count


def main():
    """Main function to analyze and plot malloc calls."""
    print("Analyzing git history for malloc calls...")

    # Get all commits
    commits = get_git_commits()
    print(f"Found {len(commits)} commits")

    if not commits:
        print("No commits found!")
        return

    # Count malloc calls for each commit
    dates = []
    counts = []

    print("Counting malloc calls per commit...")
    for i, (commit_hash, date) in enumerate(commits):
        count = count_malloc_in_commit(commit_hash)
        dates.append(date)
        counts.append(count)

        if (i + 1) % 10 == 0 or i == len(commits) - 1:
            print(f"Processed {i + 1}/{len(commits)} commits...")

    # Create plot
    print("Creating plot...")
    fig, ax = plt.subplots(figsize=(12, 6))

    ax.plot(dates, counts, marker="o", markersize=3, linewidth=1, alpha=0.7)
    ax.set_xlabel("Date", fontsize=12)
    ax.set_ylabel("Number of malloc/calloc/realloc calls", fontsize=12)
    ax.set_title("Malloc Calls per Commit Over Time", fontsize=14, fontweight="bold")
    ax.grid(True, alpha=0.3)

    # Format x-axis dates
    ax.xaxis.set_major_formatter(mdates.DateFormatter("%Y-%m-%d"))
    ax.xaxis.set_major_locator(mdates.AutoDateLocator())
    plt.xticks(rotation=45, ha="right")

    # Adjust layout
    plt.tight_layout()

    # Print statistics
    print("\nStatistics:")
    print(f"  Total commits analyzed: {len(commits)}")
    print(f"  Total malloc calls (latest): {counts[-1] if counts else 0}")
    print(f"  Maximum malloc calls: {max(counts) if counts else 0}")
    print(f"  Minimum malloc calls: {min(counts) if counts else 0}")
    print(f"  Average malloc calls: {sum(counts) / len(counts) if counts else 0:.2f}")

    # Show plot
    plt.show()


if __name__ == "__main__":
    main()

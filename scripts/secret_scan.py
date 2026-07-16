#!/usr/bin/env python3
"""Fail on likely credentials or private keys in release-candidate files."""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path


SKIP_DIRS = {".git", ".pio", ".venv", "venv", "__pycache__"}
TEXT_SUFFIXES = {
    "",
    ".c",
    ".cc",
    ".cpp",
    ".h",
    ".hpp",
    ".ini",
    ".md",
    ".py",
    ".txt",
    ".yml",
    ".yaml",
    ".json",
    ".csv",
    ".html",
    ".css",
    ".js",
    ".svg",
}

PATTERNS = [
    (
        "private key",
        re.compile(r"-----BEGIN (?:RSA |EC |OPENSSH |DSA )?PRIVATE KEY-----"),
    ),
    (
        "GitHub token",
        re.compile(r"\b(?:gh[opusr]_[A-Za-z0-9_]{20,}|github_pat_[A-Za-z0-9_]{20,})\b"),
    ),
    ("AWS access key", re.compile(r"\bAKIA[0-9A-Z]{16}\b")),
    (
        "generic assigned secret",
        re.compile(
            r"""(?ix)
            \b(api[_-]?key|access[_-]?token|auth[_-]?token|secret|password|passwd|pwd)
            \b\s*[:=]\s*
            ["']?(?!YOUR_|EXAMPLE|REPLACE|CHANGEME|REDACTED|\[REDACTED\])
            ([A-Za-z0-9+/=_!@#$%^&*.-]{8,})
            """
        ),
    ),
]

# Exact strings are documentation/examples, not secrets.
ALLOWED_SUBSTRINGS = {
    "YOUR_WIFI_SSID",
    "YOUR_WIFI_PASSWORD",
    "your-ssid",
    "your-password",
    "[REDACTED]",
}


def candidate_files(root: Path) -> list[Path]:
    try:
        tracked = subprocess.run(
            ["git", "-C", str(root), "ls-files", "-z"],
            check=True,
            capture_output=True,
        ).stdout
    except (subprocess.CalledProcessError, FileNotFoundError):
        tracked = b""

    if tracked:
        return [
            root / item.decode("utf-8", "surrogateescape")
            for item in tracked.split(b"\0")
            if item
        ]

    result: list[Path] = []
    for path in root.rglob("*"):
        if not path.is_file() or any(part in SKIP_DIRS for part in path.parts):
            continue
        result.append(path)
    return sorted(result)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", default=".")
    args = parser.parse_args()
    root = Path(args.root).resolve()

    findings: list[str] = []
    for path in candidate_files(root):
        if not path.exists() or path.stat().st_size > 2_000_000:
            continue
        if path.suffix.lower() not in TEXT_SUFFIXES:
            continue
        try:
            text = path.read_text(encoding="utf-8")
        except (UnicodeDecodeError, OSError):
            continue
        rel = path.relative_to(root)
        for number, line in enumerate(text.splitlines(), 1):
            if any(allowed in line for allowed in ALLOWED_SUBSTRINGS):
                continue
            for label, pattern in PATTERNS:
                if pattern.search(line):
                    findings.append(f"{rel}:{number}: {label}")

    if findings:
        print("Secret scan: FAIL", file=sys.stderr)
        print("\n".join(findings), file=sys.stderr)
        return 1
    print("Secret scan: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

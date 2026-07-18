#!/usr/bin/env python3
"""Repository release checks that do not require hardware."""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path


REQUIRED = [
    ".github/workflows/firmware.yml",
    ".github/platformio-requirements.txt",
    ".gitignore",
    "HARDWARE.md",
    "LICENSE",
    "README.md",
    "SECURITY.md",
    "THIRD_PARTY_NOTICES.md",
    "docs/GITHUB_METADATA.md",
    "docs/HARDWARE_LAB_CARD.md",
    "docs/PROJECT_STATUS.md",
    "docs/PROTOCOL.md",
    "docs/SOURCE_PROVENANCE.md",
    "docs/VERIFICATION.md",
    "firmware/include/board_pins.h",
    "firmware/include/wifi_config.example.h",
    "firmware/platformio.ini",
    "hardware/BOM.csv",
    "hardware/wiring-diagram.svg",
    "scripts/check_repo.py",
    "scripts/secret_scan.py",
    "scripts/verify.sh",
]
FORBIDDEN_NAMES = {
    "wifi_config.h",
    ".env",
    "id_rsa",
    "id_ed25519",
}
FORBIDDEN_DIRS = {
    ".pio",
    "__pycache__",
    ".venv",
    "venv",
    "node_modules",
}
FORBIDDEN_SUFFIXES = {
    ".o",
    ".a",
    ".elf",
    ".bin",
    ".hex",
    ".map",
    ".pyc",
    ".pyo",
}
MAX_FILE_BYTES = 5 * 1024 * 1024


def files_for_check(root: Path) -> list[Path]:
    try:
        raw = subprocess.run(
            ["git", "-C", str(root), "ls-files", "-z"],
            check=True,
            capture_output=True,
        ).stdout
    except (subprocess.CalledProcessError, FileNotFoundError):
        raw = b""
    if raw:
        return [
            root / item.decode("utf-8", "surrogateescape")
            for item in raw.split(b"\0")
            if item
        ]
    return sorted(
        path
        for path in root.rglob("*")
        if path.is_file()
        and not any(part in {".git", ".pio", "__pycache__"} for part in path.parts)
    )


def text(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="strict")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", default=".")
    args = parser.parse_args()
    root = Path(args.root).resolve()
    errors: list[str] = []

    for rel in REQUIRED:
        if not (root / rel).is_file():
            errors.append(f"missing required file: {rel}")

    files = files_for_check(root)
    for path in files:
        rel = path.relative_to(root)
        if path.name in FORBIDDEN_NAMES:
            errors.append(f"forbidden local/config file: {rel}")
        if any(part in FORBIDDEN_DIRS for part in rel.parts):
            errors.append(f"forbidden generated directory: {rel}")
        if path.suffix.lower() in FORBIDDEN_SUFFIXES:
            errors.append(f"forbidden generated artifact: {rel}")
        if path.stat().st_size > MAX_FILE_BYTES:
            errors.append(f"file exceeds 5 MiB: {rel}")

    try:
        platformio = text(root / "firmware/platformio.ini")
        if "platform = ststm32@19.5.0" not in platformio:
            errors.append("firmware platform is not pinned to ststm32@19.5.0")
    except (OSError, UnicodeError):
        errors.append("cannot read firmware/platformio.ini")

    source_checks = {
        "firmware/include/board_pins.h": [
            r"ESP-01S\s+—\s+USART1:\s+PA9 TX,\s+PA10 RX",
            r"调试串口\s+—\s+USART2:\s+PA2 TX,\s+PA3 RX",
        ],
        "firmware/include/esp_uart.h": [r"USART1（PA9/PA10）"],
        "README.md": [
            r"ESP-01S \| PA9 TX / PA10 RX \| \*\*USART1",
            r"调试串口 \| PA2 TX / PA3 RX \| \*\*USART2",
            r"尚未对当前提交做真机复测",
        ],
    }
    for rel, patterns in source_checks.items():
        try:
            content = text(root / rel)
        except (OSError, UnicodeError):
            continue
        for pattern in patterns:
            if not re.search(pattern, content):
                errors.append(f"fact contract missing in {rel}: {pattern}")

    for rel in [
        "README.md",
        "docs/PROJECT_STATUS.md",
        "docs/VERIFICATION.md",
        "docs/HARDWARE_LAB_CARD.md",
    ]:
        try:
            content = text(root / rel).lower()
        except (OSError, UnicodeError):
            continue
        forbidden_claims = [
            "hardware re-verified: pass",
            "current hardware verified",
            "system online",
        ]
        for claim in forbidden_claims:
            if claim in content:
                errors.append(f"unsupported status claim in {rel}: {claim}")

    try:
        status = text(root / "docs/PROJECT_STATUS.md")
        required_boundaries = [
            "| 真机复测 | 未执行 |",
            "不得使用“已真机验证”“生产就绪”“工业级”",
        ]
        for boundary in required_boundaries:
            if boundary not in status:
                errors.append(
                    f"project status does not preserve required boundary: {boundary}"
                )
    except (OSError, UnicodeError):
        pass

    if errors:
        print("Repository check: FAIL", file=sys.stderr)
        for error in sorted(set(errors)):
            print(f"- {error}", file=sys.stderr)
        return 1

    print(f"Repository check: PASS ({len(files)} files checked)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

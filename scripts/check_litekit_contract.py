#!/usr/bin/env python3
"""Local architecture contract checks for litekit overlay includes."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import re
import sys

import click


#==============================================================================
# public interface
#------------------------------------------------------------------------------

@dataclass(frozen=True)
class Violation:
    file_path: Path
    line_no: int
    message: str


@click.command()
@click.option(
    "--root",
    type=click.Path(exists=True, file_okay=False, dir_okay=True, path_type=Path),
    default=None,
    help="repository root (defaults to script parent parent)",
)
def main(root: Path | None) -> None:
    repo_root = _resolve_repo_root(root)
    violations = _collect_violations(repo_root)

    if violations:
        _print_fail(violations, repo_root)
        raise SystemExit(1)

    click.echo("OK: litekit architecture contract checks passed")


#==============================================================================
# private data and helpers
#------------------------------------------------------------------------------

_HEADER_SUFFIXES = {".h", ".hh", ".hpp", ".hxx", ".ipp", ".inl", ".tpp", ".ixx"}
_INCLUDE_RE = re.compile(r'^\s*#\s*include\s+"([^"]+)"')

_SYS_PROVIDER_FILES = (
    "lib/litekit-arduino/include/lite/sys/clock.h",
    "lib/litekit-arduino/include/lite/sys/uart.h",
    "lib/litekit/include/lite/sys/flash_mem.h",
    "lib/litekit-esp8266/include/lite/sys/flash_mem.h",
)


def _resolve_repo_root(root: Path | None) -> Path:
    if root is not None:
        return root.resolve()

    return Path(__file__).resolve().parents[1]


def _collect_violations(repo_root: Path) -> list[Violation]:
    violations: list[Violation] = []

    violations.extend(_check_core_platform_include_rules(repo_root))
    violations.extend(_check_no_flat_lite_includes(repo_root))
    violations.extend(_check_sys_provider_presence(repo_root))
    violations.extend(_check_include_order_contract(repo_root))

    return violations


def _iter_header_files(base_dir: Path) -> list[Path]:
    return [
        path
        for path in sorted(base_dir.rglob("*"))
        if path.is_file() and path.suffix.lower() in _HEADER_SUFFIXES
    ]


def _scan_includes(header_file: Path) -> list[tuple[int, str]]:
    includes: list[tuple[int, str]] = []

    for idx, line in enumerate(header_file.read_text(encoding="utf-8").splitlines(), start=1):
        if match := _INCLUDE_RE.match(line):
            includes.append((idx, match.group(1)))

    return includes


def _check_core_platform_include_rules(repo_root: Path) -> list[Violation]:
    violations: list[Violation] = []
    core_include = repo_root / "lib" / "litekit" / "include"

    for header_file in _iter_header_files(core_include):
        for line_no, include_path in _scan_includes(header_file):
            if include_path.startswith("lite/arduino/"):
                violations.append(
                    Violation(
                        file_path=header_file,
                        line_no=line_no,
                        message="core header includes platform path lite/arduino/*",
                    )
                )

            if include_path.startswith("lite/esp8266/"):
                violations.append(
                    Violation(
                        file_path=header_file,
                        line_no=line_no,
                        message="core header includes platform path lite/esp8266/*",
                    )
                )

    return violations


def _check_no_flat_lite_includes(repo_root: Path) -> list[Violation]:
    violations: list[Violation] = []

    include_roots = (
        repo_root / "lib" / "litekit" / "include",
        repo_root / "lib" / "litekit-arduino" / "include",
        repo_root / "lib" / "litekit-esp8266" / "include",
    )

    for include_root in include_roots:
        for header_file in _iter_header_files(include_root):
            for line_no, include_path in _scan_includes(header_file):
                if not include_path.startswith("lite/"):
                    continue

                relative = include_path[len("lite/"):]
                if "/" in relative:
                    continue

                if relative.endswith((".h", ".hh", ".hpp", ".hxx")):
                    violations.append(
                        Violation(
                            file_path=header_file,
                            line_no=line_no,
                            message=(
                                "flat include path detected; use hierarchical "
                                "lite/<domain>/<header>.h"
                            ),
                        )
                    )

    return violations


def _check_sys_provider_presence(repo_root: Path) -> list[Violation]:
    violations: list[Violation] = []

    for rel_path in _SYS_PROVIDER_FILES:
        abs_path = repo_root / rel_path
        if not abs_path.exists():
            violations.append(
                Violation(
                    file_path=Path(rel_path),
                    line_no=1,
                    message="required sys provider header not found",
                )
            )

    return violations


def _check_include_order_contract(repo_root: Path) -> list[Violation]:
    violations: list[Violation] = []

    arduino_ini = repo_root / "target-arduino.ini"
    esp_ini = repo_root / "target-esp8266.ini"

    violations.extend(
        _check_order_pair(
            file_path=arduino_ini,
            first="-I lib/litekit-arduino/include",
            second="-I lib/litekit/include",
            message=(
                "include order contract broken: litekit-arduino include root "
                "must come before litekit core include root"
            ),
        )
    )

    violations.extend(
        _check_order_pair(
            file_path=esp_ini,
            first="-I lib/litekit-esp8266/include",
            second="${arduino.build_flags}",
            message=(
                "include order contract broken: litekit-esp8266 include root "
                "must come before ${arduino.build_flags}"
            ),
        )
    )

    return violations


def _check_order_pair(
    file_path: Path,
    first: str,
    second: str,
    message: str,
) -> list[Violation]:
    text = file_path.read_text(encoding="utf-8")
    lines = text.splitlines()

    first_line = _find_line(lines, first)
    second_line = _find_line(lines, second)

    violations: list[Violation] = []

    if first_line == -1:
        violations.append(
            Violation(
                file_path=file_path,
                line_no=1,
                message=f"required entry not found: {first}",
            )
        )
        return violations

    if second_line == -1:
        violations.append(
            Violation(
                file_path=file_path,
                line_no=1,
                message=f"required entry not found: {second}",
            )
        )
        return violations

    if first_line >= second_line:
        violations.append(
            Violation(
                file_path=file_path,
                line_no=first_line,
                message=message,
            )
        )

    return violations


def _find_line(lines: list[str], needle: str) -> int:
    for idx, line in enumerate(lines, start=1):
        stripped = line.strip()
        if stripped.startswith(";") or stripped.startswith("#"):
            continue

        if needle in line:
            return idx

    return -1


def _print_fail(violations: list[Violation], repo_root: Path) -> None:
    click.echo("FAILED: litekit architecture contract violations found")

    for violation in violations:
        if violation.file_path.is_absolute():
            display_path = violation.file_path.relative_to(repo_root)
        else:
            display_path = violation.file_path

        click.echo(f"- {display_path}:{violation.line_no}: {violation.message}")


if __name__ == "__main__":
    main()

Import("env")  # type: ignore[name-defined]
env = globals()["env"]

from datetime import datetime
from pathlib import Path
import subprocess


def _run_git(project_dir, *args):
    result = subprocess.run(
        ["git", *args],
        cwd=str(project_dir),
        check=False,
        capture_output=True,
        text=True,
    )

    if result.returncode == 0:
        return result.stdout.strip()

    detail = result.stderr.strip() or result.stdout.strip()
    if not detail:
        detail = "git command failed"

    raise RuntimeError(detail)


def _parse_git_date(text):
    return datetime.fromisoformat(text)


def _head_date(project_dir):
    text = _run_git(project_dir, "show", "-s", "--format=%cI", "HEAD")
    return _parse_git_date(text)


def _month_commit_count(project_dir, year, month):
    count = 0
    text = _run_git(project_dir, "log", "--format=%cI", "HEAD")

    for line in text.splitlines():
        commit_date = _parse_git_date(line)
        if commit_date.year == year and commit_date.month == month:
            count += 1

    return count


def _short_hash(project_dir):
    return _run_git(project_dir, "rev-parse", "--short=7", "HEAD")


def _is_dirty(project_dir):
    return bool(_run_git(project_dir, "status", "--porcelain"))


def _version_info(project_dir):
    head_date = _head_date(project_dir)
    commit_count = _month_commit_count(
        project_dir,
        head_date.year,
        head_date.month,
    )

    build = max(0, commit_count - 1)
    year = head_date.year % 100
    month = head_date.month
    dirty = _is_dirty(project_dir)
    suffix = "+" if dirty else ""
    version = f"{year:02d}.{month:02d}.{build}{suffix}"

    return {
        "build": build,
        "dirty": dirty,
        "hash": _short_hash(project_dir),
        "month": month,
        "suffix": suffix,
        "version": version,
        "year": year,
    }


def _header_text(info):
    dirty = 1 if info["dirty"] else 0

    return f"""#pragma once

#define APP_VERSION_STR         "{info["version"]}"
#define APP_VERSION_YEAR        {info["year"]}
#define APP_VERSION_MONTH       {info["month"]}
#define APP_VERSION_BUILD       {info["build"]}
#define APP_VERSION_SUFFIX      "{info["suffix"]}"
#define APP_GIT_HASH            "{info["hash"]}"
#define APP_GIT_DIRTY           {dirty}
"""


def _write_if_changed(path, text):
    if path.exists() and path.read_text(encoding="ascii") == text:
        return

    path.write_text(text, encoding="ascii")


project_dir = Path(env.subst("$PROJECT_DIR"))
build_info_dir = project_dir / ".pio" / "build_info"
build_info_path = build_info_dir / "build_info.h"

env.PrependUnique(CPPPATH=[str(build_info_dir)])

try:
    info = _version_info(project_dir)
except RuntimeError as error:
    print(f"build info: cannot generate version ({error})")
    env.Exit(1)

build_info_dir.mkdir(parents=True, exist_ok=True)
_write_if_changed(build_info_path, _header_text(info))

print(f"build info: version {info['version']}")
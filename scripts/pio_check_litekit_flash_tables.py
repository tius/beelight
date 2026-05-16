Import("env")

from pathlib import Path
import re
import subprocess


# verify selected litekit lookup tables are linked into flash-mapped memory
# on esp8266 (0x4020_0000..0x402F_FFFF)
_FLASH_ADDR_START = 0x40200000
_FLASH_ADDR_END = 0x40300000

# symbols use demangled names from `xtensa-lx106-elf-nm -C`
_REQUIRED_TABLE_SYMBOLS = (
    "lite::gamma_lut_u8_to_u10_",
    "lite::BreathLut::breath_lut",
)


def _resolve_nm_path() -> str:
    nm_path = env.subst("$NM")
    if nm_path:
        return nm_path

    cc_path = Path(env.subst("$CC"))
    candidate = cc_path.with_name("xtensa-lx106-elf-nm")
    if candidate.exists():
        return str(candidate)

    exe_candidate = cc_path.with_name("xtensa-lx106-elf-nm.exe")
    if exe_candidate.exists():
        return str(exe_candidate)

    return "xtensa-lx106-elf-nm"


def _parse_symbols(nm_output: str) -> dict[str, int]:
    symbols: dict[str, int] = {}
    pattern = re.compile(r"^\s*([0-9A-Fa-f]+)\s+\w\s+(.+)$")

    for line in nm_output.splitlines():
        if match := pattern.match(line):
            address = int(match.group(1), 16)
            name = match.group(2).strip()
            symbols[name] = address

    return symbols


def _is_flash_mapped(address: int) -> bool:
    return _FLASH_ADDR_START <= address < _FLASH_ADDR_END


def _run_flash_table_check(source, target, env) -> None:
    pio_env = env.subst("$PIOENV")

    # tests may not pull all app tables into the link graph
    if pio_env.startswith("test_"):
        print("flash table check: skipped for test environment")
        return

    elf_node = target[0] if target else source[0]
    elf_path = Path(elf_node.get_abspath())
    nm_cmd = _resolve_nm_path()

    result = subprocess.run(
        [nm_cmd, "-C", "-n", str(elf_path)],
        check=False,
        text=True,
        capture_output=True,
    )

    if result.returncode != 0:
        print("flash table check: cannot inspect elf symbols")
        print(result.stderr.strip())
        env.Exit(1)

    symbols = _parse_symbols(result.stdout)
    violations: list[str] = []

    for symbol_name in _REQUIRED_TABLE_SYMBOLS:
        address = symbols.get(symbol_name)
        if address is None:
            violations.append(f'symbol not found: "{symbol_name}"')
            continue

        if not _is_flash_mapped(address):
            violations.append(
                f'symbol not flash-mapped: "{symbol_name}" at 0x{address:08X}'
            )

    if violations:
        print("FAILED: flash table placement check failed")
        for violation in violations:
            print(f"- {violation}")
        env.Exit(1)

    print("OK: flash table placement check passed")


env.AddPostAction("$PROGPATH", _run_flash_table_check)

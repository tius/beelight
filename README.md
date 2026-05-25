# beelight

## status

firmware is work in progress.

## hardware

see [docs/hardware/hardware.md](docs/hardware/hardware.md).

## build

The PlatformIO command may not be on `PATH` in a plain PowerShell terminal.
Set `$pio` first and use it for all PlatformIO commands:

```powershell
$pio = "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe"
& $pio --version
```

Build the default firmware target:

```powershell
& $pio run -e bee_rysta
```

PlatformIO runs `scripts/pio_build_info.py` before each Arduino build. The
script writes `.pio/build_info/build_info.h` and prepends that directory to the
include path. The generated firmware version uses `YY.MM.n`, where `YY.MM` is
the year and month of the `HEAD` commit and `n` is the zero-based commit count
within that month. Dirty working trees add a `+` suffix.

Use `APP_VERSION_STR` for display text, for example `26.05.72+`. For structured
logic use `APP_VERSION_YEAR`, `APP_VERSION_MONTH`, `APP_VERSION_BUILD`,
`APP_VERSION_SUFFIX`, `APP_GIT_HASH`, and `APP_GIT_DIRTY`. There is no numeric
`APP_VERSION` macro.

## tests

Set `$pio` as shown in "build" first.

Litekit tests use `.pio-litekit/build`, `.pio-litekit/libdeps`, and
`.pio-litekit/cache` so app and test builds do not share PlatformIO state.

Build litekit tests without uploading or running them:

```powershell
& $pio test --project-conf platformio-litekit.ini -e test_litekit_rysta --without-uploading --without-testing
```

Run litekit tests on the configured PlatformIO target:

```powershell
& $pio test --project-conf platformio-litekit.ini -e test_litekit_rysta
```

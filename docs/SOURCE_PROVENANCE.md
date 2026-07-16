# Source provenance

Status date: 2026-07-17

## Read-only sources reviewed

```text
/home/rongyi/桌面/数据采集系统
/mnt/shared/2026项目/数据采集系统.zip
```

The paths above are local provenance references and are not required to build
this public candidate.

## Comparison result

Before publication cleanup, the desktop directory and ZIP each contain 35
non-generated file paths. After excluding:

- `firmware/.pio/`;
- empty `firmware/.git/`;
- local-only `firmware/include/wifi_config.h`;

the sources expose 34 candidate source paths: 33 matching public paths plus
the differing `firmware/src/esp_uart.c`.

- Across the original 35 non-generated paths, 34 are byte-identical.
- Of those 34 matches, local-only `wifi_config.h` is intentionally not
  published.
- `firmware/src/esp_uart.c` differs.
- The desktop version is newer and adds debug dumps for ESP receive error,
  timeout and ready-wait states.
- Desktop firmware build output is newer than this source change.

## Candidate decision

The desktop version of `esp_uart.c` is used as the latest source candidate.
The archive remains an immutable historical baseline.

This decision indicates source recency and buildability only. It does not claim
that the newer diagnostics version has been re-tested on physical hardware.

## Credential handling

Both local sources contained a local Wi-Fi configuration file. Its values are
not reproduced here. The public candidate:

- excludes `wifi_config.h`;
- includes only `wifi_config.example.h`;
- blocks the local file in `.gitignore`;
- fails repository checks if the local file becomes part of the release
  candidate or tracked file set.

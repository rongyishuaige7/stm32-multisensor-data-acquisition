# Project status

Status date: 2026-07-17

## Evidence levels

| Level | Status | Meaning |
|---|---|---|
| Source-confirmed | PASS | Public candidate source has been compared with the local source and historical archive |
| Build-verified | PASS | Firmware builds from the example configuration; Python tools pass syntax checks |
| Historically demonstrated | UNKNOWN | No suitable dated photo or video was found in the reviewed source/archive |
| Hardware re-verified | NOT RUN | The current candidate commit has not been flashed and tested on the physical prototype |
| EDA/manufacturing available | NO | No schematic, PCB, CubeMX `.ioc`, Gerber or manufacturing source was found |

## What is confirmed

- The project contains STM32F103 firmware for four sensor inputs, W25Q64
  storage, SSD1306 output, alarms and ESP-01S AT networking.
- Before publication cleanup, the desktop source and
  `/mnt/shared/2026项目/数据采集系统.zip` each contain the same 35 non-generated
  file paths.
- Thirty-four of those source/archive paths match byte-for-byte; one is the
  differing `esp_uart.c`.
- The local-only `wifi_config.h` is deliberately not publishable and is absent
  from this candidate.
- The desktop `firmware/src/esp_uart.c` is newer and adds receive diagnostics
  for error and timeout states.
- Existing desktop build output was generated after that source change.
- A fresh isolated build from the example Wi-Fi configuration succeeds.

See [SOURCE_PROVENANCE.md](SOURCE_PROVENANCE.md) and
[VERIFICATION.md](VERIFICATION.md).

## Documentation correction made for this candidate

The historical README and a comment in `esp_uart.h` described ESP-01S on
USART2, but the executable source consistently uses:

```text
ESP-01S: USART1, PA9 TX / PA10 RX
debug:   USART2, PA2 TX / PA3 RX
```

The candidate documentation and header comment now follow the executable
source. This is a **source-level correction**. It still needs physical wiring
confirmation before the status can become Hardware re-verified.

## Known limitations

- Trusted-LAN-only TCP, with no authentication or TLS.
- Current hardware re-test not run.
- No current photos or video.
- No EDA or manufacturing files.
- FSR402 value is an uncalibrated demonstration mapping.
- E3Z-LS63 variant and voltage interface are not confirmed.
- A1104 module/part variant and pull-up interface are not confirmed.
- Offline ring overflow erases pending cached records.
- History does not wrap after the configured area fills.
- Metadata persistence is periodic and no power-fail detector is implemented.
- The optional Web gateway uses a bounded major-version dependency, not a
  fully locked environment.

## Status wording rule

Until physical evidence is added, public descriptions must use:

```text
Source-confirmed · Build-verified · Current hardware re-test not run
```

Do not use “hardware verified”, “production ready”, “industrial grade” or an
online-status badge.

## Historical media and EDA added on 2026-07-18

sanitized historical photo(s). See [MEDIA_EVIDENCE](MEDIA_EVIDENCE.md) for dates, sanitization, omissions, and evidence limits.

This publication update adds historical evidence only. Current hardware re-test not run.

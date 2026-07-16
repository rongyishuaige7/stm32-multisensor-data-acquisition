# Verification

Status date: 2026-07-17

## Verified environment

```text
PlatformIO Core: 6.1.19
Platform: ST STM32 19.5.0
Board: bluepill_f103c8
Framework: STM32Cube
STM32CubeF1 package: 1.8.6
GNU Arm toolchain: 7.2.1 / PlatformIO package 1.70201.0
```

## Local pre-publication verification

The firmware was built in an isolated `/tmp` copy:

1. local `.pio` and empty `.git` directories were excluded;
2. real `wifi_config.h` was excluded;
3. `wifi_config.example.h` was copied to the temporary local config;
4. `pio run -d firmware` completed successfully;
5. both Python tools passed `py_compile`.

Observed firmware size:

```text
RAM:   2436 / 20480 bytes (11.9%)
Flash: 28700 / 65536 bytes (43.8%)
```

Exact binary size can differ when tool packages or source change. CI is the
authoritative build result for a future remote commit.

## Reproduce locally

```bash
cp firmware/include/wifi_config.example.h firmware/include/wifi_config.h
bash scripts/verify.sh
```

`scripts/verify.sh` removes its temporary local configuration if it created
one, copies the firmware into a temporary build directory, and leaves the
candidate tree free of PlatformIO output. It does not flash hardware.

## What this proves

- Repository safety/structure checks pass.
- The candidate source compiles with the documented PlatformIO platform.
- Python files are syntactically valid.

## What this does not prove

- ST-Link upload;
- boot and watchdog behavior on a physical board;
- actual sensor values;
- W25Q64 erase/write/read and JEDEC identification;
- ESP-01S AT firmware compatibility;
- Wi-Fi join and TCP connection;
- offline recording and later replay;
- Web UI display using live hardware data;
- electrical safety of the unconfirmed sensor interfaces.

## Hardware re-test checklist

- [ ] Record the exact Git commit and test date.
- [ ] Confirm the actual STM32 board and oscillator.
- [ ] Confirm ESP-01S is wired to PA9/PA10 and debug UART to PA2/PA3.
- [ ] Confirm E3Z-LS63 full suffix, supply, NPN/PNP output and level interface.
- [ ] Confirm A1104 exact part/module, supply and pull-up.
- [ ] Confirm FSR402 divider value and mechanical setup.
- [ ] Flash through ST-Link and confirm startup.
- [ ] Observe valid DS18B20 data.
- [ ] Change FSR402 pressure and observe a value change.
- [ ] Trigger A1104 pulses and observe RPM.
- [ ] Change the photoelectric input and observe state/alarm behavior.
- [ ] Read a plausible W25Q64 JEDEC ID.
- [ ] Join Wi-Fi and start TCP port 8080.
- [ ] Receive live JSON with `tools/tcp_client.py`.
- [ ] Request history and receive `hist_done`.
- [ ] Force an offline/send-failure sample and later receive one `cached` item.
- [ ] Open the Web gateway and display live data.
- [ ] Capture redacted serial output and, if available, dated photos/video.

Only after the applicable checklist passes may
`docs/PROJECT_STATUS.md` say `Hardware re-verified`.

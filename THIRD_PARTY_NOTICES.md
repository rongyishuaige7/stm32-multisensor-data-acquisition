# Third-party notices

The repository's own source files are released under the MIT License in
[`LICENSE`](LICENSE). The build and optional Web tool use third-party
dependencies that are downloaded separately; their source code is not
vendored in this repository.

## Build dependencies

### PlatformIO Core

- Project: <https://github.com/platformio/platformio-core>
- Verified local version: 6.1.19
- License: Apache-2.0
- Role: package management and firmware build orchestration

### PlatformIO ST STM32 platform

- Project: <https://github.com/platformio/platform-ststm32>
- Pinned project version: 19.5.0
- License declared by the platform project: Apache-2.0
- Role: STM32 build integration and package selection

### STM32CubeF1

- Project: <https://github.com/STMicroelectronics/STM32CubeF1>
- Version resolved by the verified build: 1.8.6
- Relevant components in this build:
  - STM32F1 HAL: BSD-3-Clause
  - CMSIS and CMSIS Device: Apache-2.0
- Upstream component license table:
  <https://github.com/STMicroelectronics/STM32CubeF1/blob/master/LICENSE.md>
- Role: STM32F1 HAL, device headers and startup support

### GNU Arm Embedded Toolchain

- Project: <https://developer.arm.com/Tools%20and%20Software/GNU%20Toolchain>
- Version resolved by the verified build: 7.2.1
- PlatformIO package version: 1.70201.0
- Package-declared license: GPL-2.0-or-later
- Role: compiler, assembler and linker

The compiler's license does not relicense the project source. Any runtime
components linked into generated binaries remain subject to their own
licenses.

## Optional Web gateway dependency

### aiohttp

- Project: <https://github.com/aio-libs/aiohttp>
- Declared requirement: `aiohttp>=3.9,<4`
- License: Apache-2.0 AND MIT
- Role: local HTTP and WebSocket gateway

The lock-free requirement is suitable for the optional example but not
byte-for-byte reproducible. Record the installed version when publishing a
demo environment.

## Hardware and trademarks

STM32 and STM32Cube are trademarks of STMicroelectronics. ESP8266/ESP-01S
names identify compatible hardware; no endorsement is implied. Sensor and
module names belong to their respective owners.

No third-party hardware datasheet, logo, EDA file, binary toolchain or
framework source is redistributed by this repository.

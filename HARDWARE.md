# Hardware and wiring notes

本文件记录当前源码对应的引脚和安全边界。它是接线说明，不是经过电气审查的 PCB 原理图。

## 1. 当前源码引脚

| 模块/信号 | STM32F103C8T6 | 接口 | 说明 |
|---|---|---|---|
| DS18B20 DATA | PB12 | 1-Wire | `board_pins.h` |
| FSR402 divider output | PA1 | ADC1_IN1 | `board_pins.h` |
| A1104 output | PA0 | EXTI0 | `board_pins.h` |
| E3Z-LS63 output | PA4 | GPIO input | `board_pins.h` |
| Buzzer control | PB0 | GPIO output, low-active in code | `board_pins.h` / `alarm.c` |
| Mute key | PB5 | GPIO input, internal pull-up | `board_pins.h` / `alarm.c` |
| W25Q64 SCK/MISO/MOSI | PA5/PA6/PA7 | SPI1 | `board_pins.h` |
| W25Q64 CS | PB1 | GPIO output | `board_pins.h` |
| ESP-01S RX/TX | PA9 TX / PA10 RX | USART1 115200 | `board_pins.h` / `esp_uart.c` |
| Debug TX/RX | PA2 TX / PA3 RX | USART2 115200 | `board_pins.h` / `syscalls.c` |
| SSD1306 SCL/SDA | PB6/PB7 | software I²C | `board_pins.h` / `oled.c` |
| On-board LED | PC13 | GPIO output, low-active | `board_pins.h` |

## 2. 必须遵守的电源边界

### ESP-01S

- 使用稳定的 3.3 V 电源；
- 供电应能承担 Wi-Fi 发射时的瞬态电流，既有设计要求至少约 500 mA 的余量；
- `EN/CH_PD` 需要拉到 3.3 V；
- STM32 与 ESP-01S 必须共地；
- 不要把 5 V 直接接到 ESP-01S 电源或 GPIO；
- PA9（STM32 TX）接 ESP RX，PA10（STM32 RX）接 ESP TX。

### E3Z-LS63

E3Z-LS63 常见于工业光电传感器系列，但不同后缀可能有不同供电和 NPN/PNP 输出。当前资料没有确认具体后缀、供电和接口板：

- **不要**把未确认的工业传感器输出直接接 PA4；
- 先核对传感器铭牌和数据手册；
- 确保送入 STM32 的高电平不超过 3.3 V；
- 需要时使用合适的光耦、分压、晶体管或电平转换；
- 真机确认前，本仓库不提供唯一接法。

### A1104

当前资料只确认 PA0 用于上升沿计数，没有确认具体模块板、上拉电阻和供电：

- 核对实际器件/模块的输出类型；
- 上拉电压不得让 PA0 超出 STM32 输入范围；
- 若使用裸器件，按对应数据手册接线。

### FSR402

- FSR402 需要与固定电阻组成分压；
- 当前源码中的 ADC 到“克”的换算是演示映射；
- 实际分压电阻值和机械受力结构尚待实物确认；
- 不用于计量、医疗或安全判断。

### 蜂鸣器

源码按 PB0 低电平触发处理，既有设计描述为三极管驱动。晶体管型号、基极电阻和蜂鸣器额定电压尚待实物确认，不要用 GPIO 直接驱动超出额定电流的负载。

## 3. 接线图

见 [`hardware/wiring-diagram.svg`](hardware/wiring-diagram.svg)。

图中的虚线边界表示：

- 需要独立供电；
- 电压/输出类型待实物确认；
- 不是直接连接的承诺。

## 4. BOM 证据分层

[`hardware/BOM.csv`](hardware/BOM.csv) 使用以下状态：

- `source-confirmed`：源码明确引用该器件或功能；
- `document-confirmed`：既有文档明确出现，但还没有当前实物照片；
- `hardware-pending`：需要通过实物、器件铭牌或原理图确认。

## 5. 当前未提供

- EDA 源文件；
- PCB 原理图和 PCB Layout；
- Gerber 或制造文件；
- 当前提交对应的实物照片；
- 当前提交对应的演示视频。

因此本仓库不会使用“硬件已复测”或“PCB 已开源”的表述。

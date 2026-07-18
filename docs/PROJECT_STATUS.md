# 项目状态

状态日期：2026-07-17

## 证据等级

| 等级 | 状态 | 含义 |
|---|---|---|
| 源码已确认 | 通过 | 公开候选源码已与本地源码和历史归档对比 |
| 构建已验证 | 通过 | 固件可从示例配置构建；Python 工具通过语法检查 |
| 历史演示 | 未知 | 审查的源码/归档中未找到合适的带日期照片或视频 |
| 真机复测 | 未执行 | 当前候选提交尚未烧录到实体原型进行测试 |
| EDA/制造资料可用性 | 无 | 未找到原理图、PCB、CubeMX `.ioc`、Gerber 或制造源文件 |

## 已确认内容

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

## 此候选版本的文档修正

历史 README 和 `esp_uart.h` 中的注释将 ESP-01S 描述为使用
USART2，但可执行源码一致使用：

```text
ESP-01S: USART1, PA9 TX / PA10 RX
debug:   USART2, PA2 TX / PA3 RX
```

候选版本的文档和头文件注释现已与可执行源码一致。这是一次**源码级修正**；在状态可升级为“真机已复测”前，仍需确认实体接线。

## 已知限制

- Trusted-LAN-only TCP, with no authentication or TLS.
- 当前未进行真机复测。
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

## 状态表述规则

在补充实体证据前，公开描述必须使用：

```text
源码已确认 · 构建已验证 · 当前未进行真机复测
```

不得使用“已真机验证”“生产就绪”“工业级”或在线状态徽章。

## 2026-07-18 新增的历史素材与 EDA

已脱敏的历史照片。日期、脱敏处理、未公开材料和证据边界见 [MEDIA_EVIDENCE](MEDIA_EVIDENCE.md)。

本次发布仅新增历史证据。当前未进行真机复测。

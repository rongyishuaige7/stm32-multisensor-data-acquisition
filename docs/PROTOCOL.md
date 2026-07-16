# TCP JSON protocol

## Transport

- ESP-01S runs an AT-firmware TCP server.
- Default port: `8080`.
- Multiple client links are supported by the firmware.
- Each application message is one JSON object followed by `\n`.
- There is no authentication, encryption or internet-facing security layer.

Use only on a trusted LAN.

## Device to client

### Current sample

```json
{"type":"data","t":23.50,"r":120,"p":300,"l":1,"a":0,"u":42}
```

### Replayed offline sample

```json
{"type":"cached","t":23.50,"r":120,"p":300,"l":1,"a":0,"u":42}
```

### History item

```json
{"type":"hist","i":0,"t":23.50,"r":120,"p":300,"l":1,"a":0,"u":42}
```

### History completion

```json
{"type":"hist_done","from":0,"to":20}
```

### Invalid request

```json
{"type":"err","code":"bad_request"}
```

## Client to device

```json
{"cmd":"history","from":0,"to":20}
```

`from` is inclusive. The current firmware constrains and parses the range
internally; clients must wait for `hist_done` before assuming the request is
complete.

## Field meanings

| Field | Meaning | Boundary |
|---|---|---|
| `t` | DS18B20 temperature in °C | Sensor failure may be represented internally by a sentinel; validate before use |
| `r` | estimated RPM | Depends on magnet/sensor geometry |
| `p` | FSR402 prototype pressure mapping | Not calibrated mass or force |
| `l` | photoelectric digital state | Polarity depends on the actual interface |
| `a` | alarm bit mask | See `firmware/include/app_types.h` |
| `u` | uptime seconds | Not wall-clock time |
| `i` | history item index in a response | Not a globally stable record ID |

Alarm bits:

```text
bit 0: temperature
bit 1: RPM
bit 2: pressure
bit 3: light-state change
```

## Storage behavior

- A compact record occupies 12 bytes.
- One 4 KB sector is used as the offline ring.
- If that ring becomes full, the current implementation erases it and resets
  both indices; untransmitted records are lost.
- History starts at `0x100000`.
- When the history area fills, the current implementation stops appending.
- Metadata is normally persisted every 600 seconds; no power-fail detector is
  wired into the current firmware.

These behaviors are implementation facts, not durability guarantees.

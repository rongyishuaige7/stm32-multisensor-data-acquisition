# Security policy

## Scope

This repository is an educational hardware prototype, not a production data
acquisition or safety-control product.

## Network boundary

The ESP-01S starts an unauthenticated, unencrypted TCP server. The sample Web
gateway also assumes a trusted local machine and trusted LAN.

- Do not expose the TCP port or the Web gateway directly to the public
  internet.
- Use a dedicated or trusted LAN during experiments.
- Treat all incoming TCP payloads as untrusted.
- Do not use the prototype for safety-critical alarms or control.

## Credentials

`firmware/include/wifi_config.h` is local-only and blocked by `.gitignore` and
repository checks. Only `wifi_config.example.h` belongs in version control.

Never report a security issue with real Wi-Fi credentials, tokens, private
keys, personal paths, device identifiers, or private network details attached.

## Reporting

Please use GitHub's private vulnerability reporting if it is enabled for this
repository. Otherwise, open an issue containing only a minimal, redacted
description and ask for a private follow-up channel.

## Supported versions

Until the first tagged release, only the latest commit on `main` is maintained.

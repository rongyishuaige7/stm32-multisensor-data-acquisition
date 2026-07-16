#ifndef WIFI_CONFIG_EXAMPLE_TEMPLATE_H
#define WIFI_CONFIG_EXAMPLE_TEMPLATE_H

/* 复制本文件为同目录下的 wifi_config.h 并填写真实凭据（wifi_config.h 已加入 .gitignore） */

#define WIFI_SSID   "YOUR_WIFI_SSID"
#define WIFI_PASS   "YOUR_WIFI_PASSWORD"

/* TCP 服务端口；浏览器/上位机通过 nc 或网页脚本连 ESP 的 IP:此端口接收实时 JSON */
#define WIFI_TCP_PORT  8080

#endif /* WIFI_CONFIG_EXAMPLE_TEMPLATE_H */

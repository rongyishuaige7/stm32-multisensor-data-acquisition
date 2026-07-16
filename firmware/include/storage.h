#ifndef STORAGE_H
#define STORAGE_H

#include <stdbool.h>
#include "app_types.h"

void storage_init(void);
void storage_on_tick(const sensor_snapshot_t *snap, bool wifi_online);
bool storage_ring_pop(flash_record_t *out);
/** 将当前快照写入离线环形区（WiFi 在线但 TCP 发送失败时补录） */
bool storage_ring_push_snapshot(const sensor_snapshot_t *s);
void storage_persist_meta(void);
/** 强制擦写 meta 扇区（掉电前等场景；不做“与 Flash 相同则跳过”） */
void storage_flush_meta_now(void);
uint32_t storage_hist_bytes_used(void);

#endif

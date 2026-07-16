#include "storage.h"
#include "w25q64.h"
#include <string.h>

#define META_MAGIC 0x53444D53UL
#define META_VERSION 1U
#define META_ADDR 0UL

/* 离线环形区：单 4KB 扇区（避免跨扇区擦写复杂度），溢出时整扇区擦除 */
#define RING_BASE 0x1000UL
#define RING_SECTOR_BYTES 4096UL
#define RING_LAST (RING_BASE + RING_SECTOR_BYTES - 1UL)

#define HIST_BASE 0x100000UL
#define HIST_LAST (W25Q64_FLASH_SIZE - 1UL)

#define REC_BYTES sizeof(flash_record_t)

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t ring_head_i;
    uint32_t ring_tail_i;
    uint32_t hist_offs;
} storage_meta_t;

static uint32_t s_ring_head_i;
static uint32_t s_ring_tail_i;
static uint32_t s_hist_offs;
static uint32_t s_rec_cap;

static uint32_t ring_bytes_total(void)
{
    return (RING_LAST - RING_BASE + 1U);
}

/** W25Q64 页编程不能跨 256B 页边界：拆成最多两次 page_program */
static bool prog_split(uint32_t abs_addr, const uint8_t *data, uint16_t len)
{
    if (len == 0 || len > 256U)
        return false;
    uint16_t page_off = (uint16_t)(abs_addr % 256U);
    if ((uint32_t)page_off + (uint32_t)len <= 256U)
        return w25q64_page_program(abs_addr, data, len);
    uint16_t first = (uint16_t)(256U - page_off);
    if (!w25q64_page_program(abs_addr, data, first))
        return false;
    return w25q64_page_program(abs_addr + first, data + first, (uint16_t)(len - first));
}

static void meta_default(void)
{
    s_ring_head_i = 0;
    s_ring_tail_i = 0;
    s_hist_offs = 0;
}

static bool meta_load(void)
{
    storage_meta_t m;
    w25q64_read(META_ADDR, (uint8_t *)&m, sizeof(m));
    if (m.magic != META_MAGIC || m.version != META_VERSION)
        return false;
    s_rec_cap = ring_bytes_total() / REC_BYTES;
    if (m.ring_head_i >= s_rec_cap || m.ring_tail_i >= s_rec_cap)
        return false;
    if (m.hist_offs > (HIST_LAST - HIST_BASE + 1U - REC_BYTES))
        return false;
    s_ring_head_i = m.ring_head_i;
    s_ring_tail_i = m.ring_tail_i;
    s_hist_offs = m.hist_offs;
    return true;
}

/** 断电恢复后：历史偏移若落在扇区中段，向上对齐到下一扇区起点，避免在未擦除区域编程 */
static void hist_offs_align_after_load(void)
{
    if (s_hist_offs == 0U)
        return;
    uint32_t aligned = ((s_hist_offs + 4095U) / 4096U) * 4096U;
    if (aligned != s_hist_offs)
        s_hist_offs = aligned;
}

static bool ring_full(void)
{
    return (((s_ring_tail_i + 1U) % s_rec_cap) == s_ring_head_i);
}

static bool ring_empty(void)
{
    return s_ring_head_i == s_ring_tail_i;
}

static bool ring_push(const flash_record_t *r)
{
    if (s_rec_cap < 2)
        return false;
    if (ring_full()) {
        /* 环形区满：整扇区擦除，丢弃未补传缓存（可接受策略） */
        if (!w25q64_sector_erase_4k(RING_BASE))
            return false;
        s_ring_head_i = 0;
        s_ring_tail_i = 0;
    }
    uint32_t addr = RING_BASE + s_ring_tail_i * REC_BYTES;
    if (!prog_split(addr, (const uint8_t *)r, REC_BYTES))
        return false;
    s_ring_tail_i = (s_ring_tail_i + 1U) % s_rec_cap;
    return true;
}

bool storage_ring_pop(flash_record_t *out)
{
    if (!out || ring_empty())
        return false;
    uint32_t addr = RING_BASE + s_ring_head_i * REC_BYTES;
    w25q64_read(addr, (uint8_t *)out, REC_BYTES);
    s_ring_head_i = (s_ring_head_i + 1U) % s_rec_cap;
    return true;
}

static bool hist_append(const flash_record_t *r)
{
    uint32_t max_hist = (HIST_LAST - HIST_BASE + 1U);
    if (s_hist_offs + REC_BYTES > max_hist) {
        /* 写满后回到起点需整片擦除，此处简化：停止写入 */
        return false;
    }
    uint32_t addr = HIST_BASE + s_hist_offs;
    if ((addr % 4096U) == 0U) {
        if (!w25q64_sector_erase_4k(addr))
            return false;
    }
    if (!prog_split(addr, (const uint8_t *)r, REC_BYTES))
        return false;
    s_hist_offs += REC_BYTES;
    return true;
}

static void snap_to_record(const sensor_snapshot_t *s, flash_record_t *r)
{
    memset(r, 0, sizeof(*r));
    r->timestamp_s = s->uptime_s;
    r->temp_c_x100 = (int16_t)(s->temp_c * 100.0f);
    r->rpm = s->rpm;
    r->pressure_g = s->pressure_g;
    r->light_on = s->light_on;
    r->flags = s->alarm;
}

void storage_init(void)
{
    s_rec_cap = ring_bytes_total() / REC_BYTES;
    meta_default();
    uint32_t id = 0;
    if (w25q64_read_id(&id) && id != 0) {
        if (!meta_load())
            meta_default();
        else
            hist_offs_align_after_load();
    }
}

void storage_on_tick(const sensor_snapshot_t *snap, bool wifi_online)
{
    flash_record_t rec;
    snap_to_record(snap, &rec);
    (void)hist_append(&rec);
    if (!wifi_online)
        (void)ring_push(&rec);
}

bool storage_ring_push_snapshot(const sensor_snapshot_t *s)
{
    flash_record_t rec;
    snap_to_record(s, &rec);
    return ring_push(&rec);
}

static bool meta_equal_current_in_flash(void)
{
    storage_meta_t m;
    w25q64_read(META_ADDR, (uint8_t *)&m, sizeof(m));
    if (m.magic != META_MAGIC || m.version != META_VERSION)
        return false;
    return m.ring_head_i == s_ring_head_i && m.ring_tail_i == s_ring_tail_i &&
           m.hist_offs == s_hist_offs;
}

static void persist_meta_write(void)
{
    if (!w25q64_sector_erase_4k(META_ADDR))
        return;
    storage_meta_t m;
    memset(&m, 0, sizeof(m));
    m.magic = META_MAGIC;
    m.version = META_VERSION;
    m.ring_head_i = s_ring_head_i;
    m.ring_tail_i = s_ring_tail_i;
    m.hist_offs = s_hist_offs;
    (void)w25q64_page_program(META_ADDR, (const uint8_t *)&m, sizeof(m));
}

void storage_persist_meta(void)
{
    if (meta_equal_current_in_flash())
        return;
    persist_meta_write();
}

void storage_flush_meta_now(void)
{
    persist_meta_write();
}

uint32_t storage_hist_bytes_used(void)
{
    return s_hist_offs;
}

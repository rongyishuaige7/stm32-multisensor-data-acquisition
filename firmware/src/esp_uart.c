#include "esp_uart.h"
#include "board_pins.h"
#include "w25q64.h"
#include "wifi_config.h"
#include "stm32f1xx_hal_iwdg.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern UART_HandleTypeDef huart1;
extern IWDG_HandleTypeDef hiwdg;
#define ESP_HUART huart1

/* ---------------- 状态机 ---------------- */

typedef enum {
    ST_RESET = 0,
    ST_WAIT_READY,
    ST_ATE0,
    ST_AT,
    ST_CWMODE,
    ST_CWJAP,
    ST_WAIT_GOTIP,
    ST_CIPMUX,
    ST_CIPSERVER,
    ST_READY,
    ST_ERROR,
} esp_state_t;

static esp_state_t s_state;
static uint32_t s_state_ms;
static bool     s_wifi_online;
static bool     s_waiting_resp;
static char     s_ip_str[20];
static uint32_t s_last_link_poll_ms;

static const char *state_name(esp_state_t st)
{
    switch (st) {
    case ST_RESET:       return "RESET";
    case ST_WAIT_READY:  return "WAIT_READY";
    case ST_ATE0:        return "ATE0";
    case ST_AT:          return "AT";
    case ST_CWMODE:      return "CWMODE";
    case ST_CWJAP:       return "CWJAP";
    case ST_WAIT_GOTIP:  return "WAIT_GOTIP";
    case ST_CIPMUX:      return "CIPMUX";
    case ST_CIPSERVER:   return "CIPSERVER";
    case ST_READY:       return "READY";
    case ST_ERROR:       return "ERROR";
    default:             return "?";
    }
}

/** 多连接 link 0..4 活跃位图（由 URC 维护） */
static uint8_t s_active_links;

/* ---------------- 串口收发 ---------------- */

static uint8_t s_rx_byte;
static char    s_buf[512];
static volatile uint16_t s_buf_len;

static void uart_tx(const char *s, uint16_t n)
{
    HAL_UART_Transmit(&ESP_HUART, (uint8_t *)s, n, 500);
}

static void uart_tx_str(const char *s)
{
    printf("[ESP TX] %s", s);
    uart_tx(s, (uint16_t)strlen(s));
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance != USART1)
        return;
    if (s_buf_len < sizeof(s_buf) - 1U)
        s_buf[s_buf_len++] = (char)s_rx_byte;
    HAL_UART_Receive_IT(&ESP_HUART, &s_rx_byte, 1);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance != USART1)
        return;
    __HAL_UART_CLEAR_OREFLAG(huart);
    (void)HAL_UART_Receive_IT(&ESP_HUART, &s_rx_byte, 1);
}

static int buf_find(const char *needle)
{
    uint16_t n;
    size_t needle_len = strlen(needle);
    if (needle_len == 0U)
        return 0;

    __disable_irq();
    n = s_buf_len;
    __enable_irq();
    if (n < needle_len)
        return -1;

    for (uint16_t i = 0; i <= (uint16_t)(n - needle_len); i++) {
        if (memcmp(&s_buf[i], needle, needle_len) == 0)
            return (int)i;
    }
    return -1;
}

static void buf_clear(void)
{
    __disable_irq();
    s_buf_len = 0;
    __enable_irq();
}

static void debug_dump_rx(const char *tag)
{
    char local[sizeof(s_buf)];
    uint16_t n;

    __disable_irq();
    n = s_buf_len;
    if (n >= sizeof(local))
        n = sizeof(local) - 1U;
    memcpy(local, s_buf, n);
    __enable_irq();

    printf("[ESP RX %s] len=%u ", tag, (unsigned)n);
    if (n == 0U) {
        printf("<empty>\r\n");
        return;
    }

    printf("\"");
    for (uint16_t i = 0; i < n; i++) {
        unsigned char c = (unsigned char)local[i];
        if (c == '\r')
            printf("\\r");
        else if (c == '\n')
            printf("\\n");
        else if (c >= 32U && c < 127U)
            printf("%c", c);
        else
            printf("\\x%02X", (unsigned)c);
    }
    printf("\"\r\n");
}

static void buf_consume(uint16_t n)
{
    __disable_irq();
    if (n >= s_buf_len) {
        s_buf_len = 0;
    } else {
        memmove(s_buf, s_buf + n, s_buf_len - n);
        s_buf_len = (uint16_t)(s_buf_len - n);
    }
    __enable_irq();
}

static void iwdg_feed_maybe(uint32_t *last_feed, uint32_t now)
{
    if ((now - *last_feed) >= 50U) {
        (void)HAL_IWDG_Refresh(&hiwdg);
        *last_feed = now;
    }
}

/* 前向声明：cipsend 等待期间需处理 +IPD */
static uint16_t try_consume_ipd(void);
static bool cipsend_multilink(const char *body);

static void drain_ipd_once(void)
{
    uint16_t n = try_consume_ipd();
    if (n > 0U)
        buf_consume(n);
}

/** 消费从某偏移到（含）换行符的一行；若无换行则尽力消费 */
static void consume_line_from(int start)
{
    uint16_t n;
    __disable_irq();
    n = s_buf_len;
    __enable_irq();
    if (start < 0 || (uint16_t)start >= n)
        return;
    for (uint16_t i = (uint16_t)start; i < n; i++) {
        if (s_buf[i] == '\n') {
            buf_consume((uint16_t)(i + 1U));
            return;
        }
    }
    buf_consume(n);
}

static int line_end_from(int start)
{
    uint16_t n;
    __disable_irq();
    n = s_buf_len;
    __enable_irq();
    if (start < 0 || (uint16_t)start >= n)
        return -1;
    for (uint16_t i = (uint16_t)start; i < n; i++) {
        if (s_buf[i] == '\n')
            return (int)(i + 1U);
    }
    return (int)n;
}

static int urc_match_bounded(const char *suffix, unsigned id)
{
    char pat[16];
    (void)snprintf(pat, sizeof(pat), "%u,%s", id, suffix);
    int pos = buf_find(pat);
    if (pos < 0)
        return -1;
    if (pos > 0 && isdigit((unsigned char)s_buf[pos - 1]))
        return -1;
    return pos;
}

/** 扫描并消费 0..4 的 CONNECT/CLOSED URC */
static void scan_link_urcs(void)
{
    for (unsigned tries = 0; tries < 8U; tries++) {
        int best_pos = -1;
        int best_end = -1;
        int best_kind = -1; /* 0=CLOSED 1=CONNECT */
        unsigned best_id = 0;

        for (unsigned id = 0; id < 5U; id++) {
            int pc = urc_match_bounded("CONNECT", id);
            if (pc >= 0) {
                int end = line_end_from(pc);
                if (best_pos < 0 || pc < best_pos) {
                    best_pos = pc;
                    best_end = end;
                    best_kind = 1;
                    best_id = id;
                }
            }
            int pd = urc_match_bounded("CLOSED", id);
            if (pd >= 0) {
                int end = line_end_from(pd);
                if (best_pos < 0 || pd < best_pos) {
                    best_pos = pd;
                    best_end = end;
                    best_kind = 0;
                    best_id = id;
                }
            }
        }
        if (best_end < 0)
            break;
        if (best_pos < 0)
            break;
        if (best_kind == 1) {
            s_active_links |= (uint8_t)(1U << best_id);
            printf("[ESP] link %u CONNECT\r\n", best_id);
        } else {
            s_active_links &= (uint8_t) ~(1U << best_id);
            printf("[ESP] link %u CLOSED\r\n", best_id);
        }
        buf_consume((uint16_t)best_end);
    }
}

static void poll_link_status(void)
{
    uart_tx_str("AT+CIPSTATUS\r\n");

    uint32_t t0 = HAL_GetTick();
    uint32_t lf = t0;
    while ((HAL_GetTick() - t0) < 250U) {
        uint32_t now = HAL_GetTick();
        iwdg_feed_maybe(&lf, now);
        scan_link_urcs();
        if (buf_find("\r\nOK") >= 0 || buf_find("\nOK") >= 0)
            break;
    }

    uint8_t found = 0;
    for (unsigned id = 0; id < 5U; id++) {
        char pat[20];
        (void)snprintf(pat, sizeof(pat), "+CIPSTATUS:%u,", id);
        if (buf_find(pat) >= 0)
            found |= (uint8_t)(1U << id);
    }

    if (found != 0U) {
        uint8_t newly_seen = (uint8_t)(found & (uint8_t)~s_active_links);
        s_active_links |= found;
        for (unsigned id = 0; id < 5U; id++) {
            if ((newly_seen & (uint8_t)(1U << id)) != 0U)
                printf("[ESP] link %u CONNECT (status)\r\n", id);
        }
    }

    buf_clear();
}

/* ---------------- AT 状态机 ---------------- */

static void enter(esp_state_t st)
{
    if (st != s_state)
        printf("[ESP] -> %s\r\n", state_name(st));
    s_state = st;
    s_state_ms = HAL_GetTick();
    s_waiting_resp = false;
    buf_clear();
}

void esp_uart_init(void)
{
    s_state = ST_RESET;
    s_state_ms = 0;
    s_wifi_online = false;
    s_waiting_resp = false;
    s_active_links = 0;
    s_last_link_poll_ms = 0;
    s_buf_len = 0;
    HAL_UART_Receive_IT(&ESP_HUART, &s_rx_byte, 1);
}

bool esp_uart_wifi_online(void)
{
    return s_wifi_online;
}

const char *esp_uart_get_ip(void)
{
    return s_ip_str;
}

static int wait_resp(uint32_t timeout_ms)
{
    /* ESP 返回有时可能与调试输出交错显示；按 AT 响应行识别 OK。 */
    if (buf_find("\r\nOK\r\n") >= 0 || buf_find("\nOK\n") >= 0 ||
        buf_find("\r\nOK\n") >= 0 || buf_find("\nOK\r") >= 0) {
        buf_clear();
        return 1;
    }
    if (buf_find("\r\nERROR") >= 0 || buf_find("\r\nFAIL") >= 0 ||
        buf_find("\nERROR") >= 0 || buf_find("\nFAIL") >= 0) {
        debug_dump_rx("ERROR");
        return -1;
    }
    if ((HAL_GetTick() - s_state_ms) > timeout_ms) {
        debug_dump_rx("TIMEOUT");
        return -1;
    }
    return 0;
}

void esp_uart_tick(void)
{
    uint32_t now = HAL_GetTick();
    uint32_t elapsed = now - s_state_ms;

    switch (s_state) {
    case ST_RESET:
        uart_tx_str("AT+RST\r\n");
        enter(ST_WAIT_READY);
        break;

    case ST_WAIT_READY:
        if (buf_find("ready") >= 0) {
            HAL_Delay(200);
            enter(ST_ATE0);
        } else if (elapsed > 5000U) {
            debug_dump_rx("WAIT_READY");
            enter(ST_RESET);
        }
        break;

    case ST_ATE0:
        if (!s_waiting_resp) {
            uart_tx_str("ATE0\r\n");
            s_state_ms = now;
            s_waiting_resp = true;
        } else {
            int r = wait_resp(2000U);
            if (r == 1)
                enter(ST_AT);
            else if (r < 0)
                enter(ST_ERROR);
        }
        break;

    case ST_AT:
        if (!s_waiting_resp) {
            uart_tx_str("AT\r\n");
            s_state_ms = now;
            s_waiting_resp = true;
        } else {
            int r = wait_resp(2000U);
            if (r == 1)
                enter(ST_CWMODE);
            else if (r < 0)
                enter(ST_ERROR);
        }
        break;

    case ST_CWMODE:
        if (!s_waiting_resp) {
            uart_tx_str("AT+CWMODE=1\r\n");
            s_state_ms = now;
            s_waiting_resp = true;
        } else {
            int r = wait_resp(3000U);
            if (r == 1)
                enter(ST_CWJAP);
            else if (r < 0)
                enter(ST_ERROR);
        }
        break;

    case ST_CWJAP:
        if (!s_waiting_resp) {
            char cmd[128];
            snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"\r\n", WIFI_SSID, WIFI_PASS);
            uart_tx_str(cmd);
            s_state_ms = now;
            s_waiting_resp = true;
        } else {
            int r = wait_resp(25000U);
            if (r == 1)
                enter(ST_CIPMUX);
            else if (r < 0)
                enter(ST_ERROR);
        }
        break;

    case ST_CIPMUX:
        if (!s_waiting_resp) {
            /* 顺便拉一次 IP 打印到调试串口 */
            uart_tx_str("AT+CIFSR\r\n");
            HAL_Delay(200);
            uint16_t bn;
            __disable_irq(); bn = s_buf_len; __enable_irq();
            if (bn < sizeof(s_buf)) s_buf[bn] = '\0';
            char *ip = strstr(s_buf, "STAIP,\"");
            if (ip) {
                ip += 7;
                char *q = strchr(ip, '"');
                if (q) {
                    *q = '\0';
                    snprintf(s_ip_str, sizeof(s_ip_str), "%s", ip);
                    printf("[ESP] IP=%s  (TCP port %u)\r\n", ip, (unsigned)WIFI_TCP_PORT);
                    *q = '"';
                }
            }
            buf_clear();

            uart_tx_str("AT+CIPMUX=1\r\n");
            s_state_ms = now;
            s_waiting_resp = true;
        } else {
            int r = wait_resp(3000U);
            if (r == 1)
                enter(ST_CIPSERVER);
            else if (r < 0)
                enter(ST_ERROR);
        }
        break;

    case ST_CIPSERVER:
        if (!s_waiting_resp) {
            char cmd[40];
            snprintf(cmd, sizeof(cmd), "AT+CIPSERVER=1,%u\r\n", (unsigned)WIFI_TCP_PORT);
            uart_tx_str(cmd);
            s_state_ms = now;
            s_waiting_resp = true;
        } else {
            int r = wait_resp(3000U);
            if (r == 1) {
                enter(ST_READY);
                s_wifi_online = true;
            } else if (r < 0) {
                enter(ST_ERROR);
            }
        }
        break;

    case ST_READY:
        if (s_active_links == 0U && (now - s_last_link_poll_ms) >= 3000U) {
            s_last_link_poll_ms = now;
            poll_link_status();
        }
        break;

    case ST_ERROR:
        s_wifi_online = false;
        s_active_links = 0;
        if (elapsed > 5000U)
            enter(ST_RESET);
        break;

    case ST_WAIT_GOTIP:
    default:
        enter(ST_RESET);
        break;
    }
}

static bool cipsend_link(int link_id, const char *body)
{
    uint16_t len = (uint16_t)strlen(body);
    if (len == 0)
        return false;

    char hdr[48];
    snprintf(hdr, sizeof(hdr), "AT+CIPSEND=%d,%u\r\n", link_id, (unsigned)len);
    uart_tx_str(hdr);

    uint32_t t0 = HAL_GetTick();
    uint32_t lf = t0;
    bool got_prompt = false;
    while ((HAL_GetTick() - t0) < 500U) {
        uint32_t now = HAL_GetTick();
        iwdg_feed_maybe(&lf, now);
        drain_ipd_once();
        scan_link_urcs();

        int gt = buf_find(">");
        if (gt >= 0) {
            buf_consume((uint16_t)(gt + 1));
            uart_tx(body, len);
            got_prompt = true;
            break;
        }
        if (buf_find("ERROR") >= 0 || buf_find("busy") >= 0 ||
            buf_find("link is not valid") >= 0) {
            buf_clear();
            return false;
        }
    }
    if (!got_prompt) {
        buf_clear();
        return false;
    }

    t0 = HAL_GetTick();
    lf = t0;
    while ((HAL_GetTick() - t0) < 500U) {
        uint32_t now = HAL_GetTick();
        iwdg_feed_maybe(&lf, now);
        drain_ipd_once();
        scan_link_urcs();

        int ok = buf_find("SEND OK");
        if (ok >= 0) {
            consume_line_from(ok);
            return true;
        }
        int fl = buf_find("SEND FAIL");
        if (fl >= 0) {
            consume_line_from(fl);
            return false;
        }
        if (buf_find("ERROR") >= 0 || buf_find("busy") >= 0) {
            buf_clear();
            return false;
        }
    }
    buf_clear();
    return false;
}

static bool cipsend_multilink(const char *body)
{
    uint8_t mask = s_active_links;
    if (mask == 0U)
        return false;
    bool any_ok = false;
    for (int id = 0; id < 5; id++) {
        if ((mask & (uint8_t)(1U << id)) == 0U)
            continue;
        if (cipsend_link(id, body)) {
            any_ok = true;
        } else {
            s_active_links &= (uint8_t) ~(1U << id);
        }
    }
    return any_ok;
}

static void send_err_bad_request(void)
{
    static const char err[] = "{\"type\":\"err\",\"code\":\"bad_request\"}\n";
    (void)cipsend_multilink(err);
}

static int32_t temp_c_to_x100(float temp_c)
{
    float scaled = temp_c * 100.0f;
    return (int32_t)(scaled >= 0.0f ? scaled + 0.5f : scaled - 0.5f);
}

static void fmt_x100(char *out, size_t out_size, int32_t value)
{
    const char *sign = "";
    uint32_t abs_value;

    if (value < 0) {
        sign = "-";
        abs_value = (uint32_t)(-value);
    } else {
        abs_value = (uint32_t)value;
    }

    (void)snprintf(out, out_size, "%s%lu.%02lu",
                   sign,
                   (unsigned long)(abs_value / 100U),
                   (unsigned long)(abs_value % 100U));
}

bool esp_uart_send_data_json(const sensor_snapshot_t *s)
{
    if (s_state != ST_READY)
        return false;
    char b[160];
    char temp[16];
    fmt_x100(temp, sizeof(temp), temp_c_to_x100(s->temp_c));
    snprintf(b, sizeof(b),
             "{\"type\":\"data\",\"t\":%s,\"r\":%u,\"p\":%u,\"l\":%u,\"a\":%u,\"u\":%lu}\n",
             temp, (unsigned)s->rpm, (unsigned)s->pressure_g,
             (unsigned)s->light_on, (unsigned)s->alarm, (unsigned long)s->uptime_s);
    return cipsend_multilink(b);
}

bool esp_uart_send_cached_record_json(const flash_record_t *r)
{
    if (s_state != ST_READY)
        return false;
    char b[160];
    char temp[16];
    fmt_x100(temp, sizeof(temp), r->temp_c_x100);
    snprintf(b, sizeof(b),
             "{\"type\":\"cached\",\"t\":%s,\"r\":%u,\"p\":%u,\"l\":%u,\"a\":%u,\"u\":%lu}\n",
             temp, (unsigned)r->rpm, (unsigned)r->pressure_g,
             (unsigned)r->light_on, (unsigned)r->flags,
             (unsigned long)r->timestamp_s);
    return cipsend_multilink(b);
}

static volatile uint8_t s_history_busy;

static void process_history_payload(const char *line)
{
    /* 防止递归：cipsend_link 内部的 drain_ipd_once 可能再次触发本函数 */
    if (s_history_busy)
        return;
    s_history_busy = 1;

    unsigned from = 0, to = 0;
    const char *pf = strstr(line, "\"from\"");
    const char *pt = strstr(line, "\"to\"");
    if (pf) {
        const char *q = strchr(pf, ':');
        if (q)
            from = (unsigned)strtoul(q + 1, NULL, 10);
    }
    if (pt) {
        const char *q = strchr(pt, ':');
        if (q)
            to = (unsigned)strtoul(q + 1, NULL, 10);
    }

    if (to < from) {
        send_err_bad_request();
        s_history_busy = 0;
        return;
    }
    if ((to - from) > 20U)
        to = from + 20U;

    for (unsigned i = from; i <= to; i++) {
        (void)HAL_IWDG_Refresh(&hiwdg);

        uint32_t addr = 0x100000UL + (uint32_t)i * sizeof(flash_record_t);
        if (addr + sizeof(flash_record_t) > W25Q64_FLASH_SIZE)
            break;
        flash_record_t r;
        w25q64_read(addr, (uint8_t *)&r, sizeof(r));
        char body[180];
        char temp[16];
        fmt_x100(temp, sizeof(temp), r.temp_c_x100);
        snprintf(body, sizeof(body),
                 "{\"type\":\"hist\",\"i\":%u,\"t\":%s,\"r\":%u,\"p\":%u,\"l\":%u,\"a\":%u,\"u\":%lu}\n",
                 i, temp, (unsigned)r.rpm, (unsigned)r.pressure_g,
                 (unsigned)r.light_on, (unsigned)r.flags,
                 (unsigned long)r.timestamp_s);
        if (!cipsend_multilink(body))
            break;

        HAL_Delay(50);
    }

    (void)HAL_IWDG_Refresh(&hiwdg);

    char done[96];
    snprintf(done, sizeof(done), "{\"type\":\"hist_done\",\"from\":%u,\"to\":%u}\n", from, to);
    (void)cipsend_multilink(done);

    s_history_busy = 0;
}

static uint16_t try_consume_ipd(void)
{
    int idx = buf_find("+IPD,");
    if (idx < 0)
        return 0;

    uint16_t nlen;
    __disable_irq();
    nlen = s_buf_len;
    __enable_irq();
    if (nlen < sizeof(s_buf))
        s_buf[nlen] = '\0';

    char *colon = strchr(s_buf + idx, ':');
    if (!colon)
        return 0;

    int len = 0;
    if (sscanf(s_buf + idx, "+IPD,%*d,%d:", &len) != 1) {
        /* 逐字节前移，避免畸形头卡死 */
        return 1U;
    }

    uint16_t header_len = (uint16_t)((colon - s_buf) - idx + 1);
    uint16_t need = (uint16_t)(idx + header_len + (uint16_t)len);
    if (s_buf_len < need)
        return 0;

    if (len > 255) {
        send_err_bad_request();
        return need;
    }

    char payload[256];
    uint16_t copy = (uint16_t)len;
    memcpy(payload, colon + 1, copy);
    payload[copy] = '\0';

    if (strstr(payload, "\"cmd\":\"history\"") != NULL || strstr(payload, "history") != NULL)
        process_history_payload(payload);

    return need;
}

void esp_uart_pump_rx(void)
{
    for (unsigned k = 0; k < 8U; k++) {
        uint16_t n = try_consume_ipd();
        if (n == 0U)
            break;
        buf_consume(n);
    }

    scan_link_urcs();

    int disc = buf_find("WIFI DISCONNECT");
    if (disc >= 0) {
        s_wifi_online = false;
        s_active_links = 0;
        if (s_state == ST_READY || s_state == ST_WAIT_GOTIP)
            enter(ST_RESET);
        else
            consume_line_from(disc);
    }

    uint16_t blen;
    __disable_irq();
    blen = s_buf_len;
    __enable_irq();
    if (blen > sizeof(s_buf) - 16U)
        buf_consume(blen / 2U);
}

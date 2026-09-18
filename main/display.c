#include "display.h"

#include <string.h>
#include "esp_log.h"
#include "led_strip.h"
#include "led_strip_rmt.h"
#include "xf_config.h"

static const char *TAG = "display";
static led_strip_handle_t s_strip;
static xf_rgb_t s_fb[XF_LED_HEIGHT][XF_LED_WIDTH];
static uint8_t s_brightness = XF_LED_DEFAULT_BRIGHTNESS;
static uint8_t s_rotation;

const xf_rgb_t XF_COLOR_BLACK  = {0, 0, 0};
const xf_rgb_t XF_COLOR_WHITE  = {255, 255, 255};
const xf_rgb_t XF_COLOR_RED    = {255, 0, 0};
const xf_rgb_t XF_COLOR_GREEN  = {0, 255, 0};
const xf_rgb_t XF_COLOR_BLUE   = {0, 0, 255};
const xf_rgb_t XF_COLOR_YELLOW = {255, 180, 0};
const xf_rgb_t XF_COLOR_CYAN   = {0, 255, 255};
const xf_rgb_t XF_COLOR_ORANGE = {255, 70, 0};
const xf_rgb_t XF_COLOR_PURPLE = {180, 0, 255};
const xf_rgb_t XF_COLOR_DIM    = {20, 20, 20};

static void rotate_xy(int x, int y, int *rx, int *ry)
{
    switch (s_rotation & 3U) {
    case 1: *rx = 7 - y; *ry = x; break;
    case 2: *rx = 7 - x; *ry = 7 - y; break;
    case 3: *rx = y; *ry = 7 - x; break;
    default: *rx = x; *ry = y; break;
    }
}

/* Physical wiring: row-major serpentine / Z-shaped 8x8 matrix. */
static int physical_index(int x, int y)
{
    int rx, ry;
    rotate_xy(x, y, &rx, &ry);
    return ry * 8 + ((ry & 1) ? (7 - rx) : rx);
}

esp_err_t display_init(void)
{
    led_strip_config_t strip_cfg = {
        .strip_gpio_num = XF_PIN_WS2812,
        .max_leds = XF_LED_COUNT,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .flags = { .invert_out = false },
    };
    led_strip_rmt_config_t rmt_cfg = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
        .mem_block_symbols = 64,
        .flags = { .with_dma = false },
    };

    esp_err_t err = led_strip_new_rmt_device(&strip_cfg, &rmt_cfg, &s_strip);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "failed to create WS2812 RMT device: %s", esp_err_to_name(err));
        return err;
    }
    display_clear();
    return display_show();
}

void display_deinit(void)
{
    if (s_strip) {
        led_strip_clear(s_strip);
        led_strip_del(s_strip);
        s_strip = NULL;
    }
}

void display_clear(void)
{
    memset(s_fb, 0, sizeof(s_fb));
}

void display_fill(xf_rgb_t color)
{
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            s_fb[y][x] = color;
        }
    }
}

void display_set_pixel(int x, int y, xf_rgb_t color)
{
    if ((unsigned)x >= 8U || (unsigned)y >= 8U) {
        return;
    }
    s_fb[y][x] = color;
}

xf_rgb_t display_get_pixel(int x, int y)
{
    if ((unsigned)x >= 8U || (unsigned)y >= 8U) {
        return XF_COLOR_BLACK;
    }
    return s_fb[y][x];
}

void display_draw_mono_bitmap(const uint8_t rows[8], xf_rgb_t on, xf_rgb_t off)
{
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            display_set_pixel(x, y, (rows[y] & (1U << (7 - x))) ? on : off);
        }
    }
}

void display_set_brightness(uint8_t brightness)
{
    s_brightness = brightness;
}

uint8_t display_get_brightness(void)
{
    return s_brightness;
}

void display_set_rotation(uint8_t quarter_turns)
{
    s_rotation = quarter_turns & 3U;
}

static uint8_t effective_scale(void)
{
    /*
     * Conservative WS2812 estimate: 20 mA per fully-on color channel.
     * Apply the user brightness first, then cap the whole frame to XF_LED_MAX_MA.
     */
    uint32_t component_sum = 0;
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            component_sum += s_fb[y][x].r + s_fb[y][x].g + s_fb[y][x].b;
        }
    }
    uint32_t est_ma = (component_sum * 20U * s_brightness) / (255U * 255U);
    if (est_ma <= XF_LED_MAX_MA || est_ma == 0) {
        return s_brightness;
    }
    uint32_t limited = ((uint32_t)s_brightness * XF_LED_MAX_MA) / est_ma;
    return limited > 255U ? 255U : (uint8_t)limited;
}

esp_err_t display_show(void)
{
    if (!s_strip) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t scale = effective_scale();
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            xf_rgb_t c = s_fb[y][x];
            uint8_t r = ((uint16_t)c.r * scale) / 255U;
            uint8_t g = ((uint16_t)c.g * scale) / 255U;
            uint8_t b = ((uint16_t)c.b * scale) / 255U;
            ESP_ERROR_CHECK(led_strip_set_pixel(s_strip, physical_index(x, y), r, g, b));
        }
    }
    return led_strip_refresh(s_strip);
}

void display_off(void)
{
    display_clear();
    if (s_strip) {
        led_strip_clear(s_strip);
    }
}

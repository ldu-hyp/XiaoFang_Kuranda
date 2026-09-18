#include "game_internal.h"

#include <string.h>
#include "esp_random.h"
#include "buzzer.h"
#include "display.h"
#include "storage.h"

typedef struct {
    xf_point_t body[64];
    uint8_t length;
    xf_dir_t dir;
    xf_point_t food;
    uint32_t elapsed_ms;
    uint16_t score;
    uint16_t high;
    bool over;
} snake_state_t;

static snake_state_t s;

static bool same(xf_point_t a, xf_point_t b) { return a.x == b.x && a.y == b.y; }

static bool on_body(xf_point_t p, uint8_t count)
{
    for (uint8_t i = 0; i < count; ++i) if (same(p, s.body[i])) return true;
    return false;
}

static void spawn_food(void)
{
    for (int tries = 0; tries < 200; ++tries) {
        xf_point_t p = { esp_random() % 8, esp_random() % 8 };
        if (!on_body(p, s.length)) { s.food = p; return; }
    }
}

static esp_err_t start(void)
{
    memset(&s, 0, sizeof(s));
    s.length = 3;
    s.body[0] = (xf_point_t){4,3};
    s.body[1] = (xf_point_t){4,4};
    s.body[2] = (xf_point_t){4,5};
    s.dir = XF_DIR_UP;
    s.high = storage_get_high_score(XF_SCORE_SNAKE);
    spawn_food();
    return ESP_OK;
}

static xf_point_t next_head(void)
{
    xf_point_t p = s.body[0];
    if (s.dir == XF_DIR_UP) --p.y;
    else if (s.dir == XF_DIR_DOWN) ++p.y;
    else if (s.dir == XF_DIR_LEFT) --p.x;
    else if (s.dir == XF_DIR_RIGHT) ++p.x;
    return p;
}

static bool opposite(xf_dir_t a, xf_dir_t b)
{
    return (a == XF_DIR_UP && b == XF_DIR_DOWN) ||
           (a == XF_DIR_DOWN && b == XF_DIR_UP) ||
           (a == XF_DIR_LEFT && b == XF_DIR_RIGHT) ||
           (a == XF_DIR_RIGHT && b == XF_DIR_LEFT);
}

static void step(void)
{
    xf_point_t next = next_head();
    bool grow = same(next, s.food);

    if (next.x < 0 || next.x >= 8 || next.y < 0 || next.y >= 8) {
        s.over = true;
        return;
    }

    /*
     * Corrected from the Rust rewrite: collision is tested against NEXT head.
     * If we are not growing, the current tail leaves this tick and may safely
     * be entered by the new head.
     */
    uint8_t collision_count = grow ? s.length : (s.length ? s.length - 1 : 0);
    if (on_body(next, collision_count)) {
        s.over = true;
        return;
    }

    uint8_t end = grow && s.length < 64 ? s.length : s.length - 1;
    for (int i = end; i > 0; --i) s.body[i] = s.body[i-1];
    s.body[0] = next;

    if (grow) {
        if (s.length < 64) ++s.length;
        ++s.score;
        buzzer_score();
        spawn_food();
    }

    if (s.over && s.score > s.high) storage_set_high_score(XF_SCORE_SNAKE, s.score);
}

static void update(const xf_input_t *in, uint32_t dt)
{
    if (s.over) return;
    if (in->dir_changed && in->dir != XF_DIR_NONE && !opposite(in->dir, s.dir)) {
        s.dir = in->dir;
    }

    s.elapsed_ms += dt;
    uint32_t interval = 520U;
    if (s.score < 25) interval -= s.score * 14U;
    if (interval < 140U) interval = 140U;
    if (s.elapsed_ms >= interval) {
        s.elapsed_ms = 0;
        step();
        if (s.over) {
            if (s.score > s.high) storage_set_high_score(XF_SCORE_SNAKE, s.score);
            buzzer_game_over();
        }
    }
}

static void render(void)
{
    display_clear();
    display_set_pixel(s.food.x, s.food.y, XF_COLOR_RED);
    for (uint8_t i = 0; i < s.length; ++i) {
        display_set_pixel(s.body[i].x, s.body[i].y, i == 0 ? XF_COLOR_GREEN : XF_COLOR_WHITE);
    }
    if (s.over) {
        for (int i=0;i<8;++i) {
            display_set_pixel(i,i,XF_COLOR_RED);
            display_set_pixel(7-i,i,XF_COLOR_RED);
        }
    }
}

static bool finished(void) { return s.over; }
static void stop(void) {}

const game_module_t *game_snake_module(void)
{
    static const game_module_t m = {
        .name="snake", .start=start, .update=update, .render=render,
        .finished=finished, .stop=stop
    };
    return &m;
}

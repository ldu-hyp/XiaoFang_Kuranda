#include "game_internal.h"

#include <string.h>
#include "esp_random.h"
#include "display.h"

typedef struct {
    uint8_t value;
    uint32_t rolling_ms;
    uint32_t frame_ms;
} dice_state_t;

static dice_state_t s;

static esp_err_t start(void)
{
    memset(&s,0,sizeof(s));
    s.value = 1;
    return ESP_OK;
}

static void update(const xf_input_t *in, uint32_t dt)
{
    if (in->shake) {
        s.rolling_ms = 650;
        s.frame_ms = 0;
    }
    if (s.rolling_ms) {
        if (dt >= s.rolling_ms) s.rolling_ms = 0;
        else s.rolling_ms -= dt;
        s.frame_ms += dt;
        if (s.frame_ms >= 70) {
            s.frame_ms = 0;
            s.value = (esp_random() % 6) + 1;
        }
    }
}

static void pip(int x,int y) {
    display_set_pixel(x,y,XF_COLOR_RED);
    display_set_pixel(x+1,y,XF_COLOR_ORANGE);
}

static void render(void)
{
    display_clear();
    uint8_t v=s.value;
    if (v==1 || v==3 || v==5) pip(3,3);
    if (v>=2) { pip(1,1); pip(5,5); }
    if (v>=4) { pip(5,1); pip(1,5); }
    if (v==6) { pip(1,3); pip(5,3); }
}

static bool finished(void){return false;}
static void stop(void){}

const game_module_t *game_dice_module(void)
{
    static const game_module_t m={.name="dice",.start=start,.update=update,.render=render,.finished=finished,.stop=stop};
    return &m;
}

#include "game_internal.h"

#include <string.h>
#include "esp_random.h"
#include "display.h"

typedef struct {
    uint8_t value;
    uint32_t rolling_ms;
    uint32_t frame_ms;
} bagua_state_t;
static bagua_state_t s;

/* bottom, middle, top: 1=solid(Yang), 0=broken(Yin) */
static const uint8_t TRIGRAMS[8] = {
    0b111, /* Qian */
    0b000, /* Kun  */
    0b001, /* Zhen */
    0b100, /* Gen  */
    0b101, /* Li   */
    0b010, /* Kan  */
    0b110, /* Dui  */
    0b011, /* Xun  */
};

static esp_err_t start(void){ memset(&s,0,sizeof(s)); return ESP_OK; }

static void update(const xf_input_t *in,uint32_t dt)
{
    if(in->shake){s.rolling_ms=700;s.frame_ms=0;}
    if(s.rolling_ms){
        s.rolling_ms = dt>=s.rolling_ms ? 0 : s.rolling_ms-dt;
        s.frame_ms += dt;
        if(s.frame_ms>=80){s.frame_ms=0;s.value=esp_random()%8;}
    }
}

static void render(void)
{
    display_clear();
    uint8_t tri=TRIGRAMS[s.value&7];
    for(int line=0;line<3;++line){
        int y=1+line*2;
        bool solid=(tri>>(2-line))&1;
        xf_rgb_t c = (line==0)?XF_COLOR_YELLOW:(line==1?XF_COLOR_ORANGE:XF_COLOR_WHITE);
        if(solid){
            for(int x=0;x<8;++x) display_set_pixel(x,y,c);
        }else{
            for(int x=0;x<3;++x) display_set_pixel(x,y,c);
            for(int x=5;x<8;++x) display_set_pixel(x,y,c);
        }
        for(int x=0;x<8;++x) display_set_pixel(x,y+1, display_get_pixel(x,y));
    }
}

static bool finished(void){return false;}
static void stop(void){}

const game_module_t *game_bagua_module(void)
{
    static const game_module_t m={.name="bagua",.start=start,.update=update,.render=render,.finished=finished,.stop=stop};
    return &m;
}

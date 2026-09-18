#include "game_internal.h"

#include <string.h>
#include "esp_random.h"
#include "buzzer.h"
#include "display.h"

typedef struct {
    bool sand[8][8];
    uint32_t elapsed_ms;
    uint32_t alarm_ms;
    bool complete;
} hourglass_state_t;
static hourglass_state_t s;

static bool allowed(int x,int y)
{
    if((unsigned)x>=8U || (unsigned)y>=8U) return false;
    int inset = y < 4 ? y : 7-y;
    return x >= inset && x <= 7-inset;
}

static esp_err_t start(void)
{
    memset(&s,0,sizeof(s));
    for(int y=0;y<4;++y)
        for(int x=0;x<8;++x)
            if(allowed(x,y)) s.sand[y][x]=true;
    return ESP_OK;
}

static bool has_upper_sand(void)
{
    for(int y=0;y<4;++y) for(int x=0;x<8;++x) if(s.sand[y][x]) return true;
    return false;
}

static void fall_step(void)
{
    bool moved=false;
    for(int y=6;y>=0;--y){
        int start = esp_random() & 1 ? 7 : 0;
        int step = start ? -1 : 1;
        for(int n=0;n<8;++n){
            int x=start+n*step;
            if(!s.sand[y][x]) continue;
            int options[3]={x, x-1, x+1};
            if(esp_random()&1){int t=options[1];options[1]=options[2];options[2]=t;}
            for(int k=0;k<3;++k){
                int nx=options[k], ny=y+1;
                if(allowed(nx,ny) && !s.sand[ny][nx]){
                    s.sand[y][x]=false;
                    s.sand[ny][nx]=true;
                    moved=true;
                    break;
                }
            }
        }
    }
    if(!has_upper_sand()){
        s.complete=true;
        s.alarm_ms=0;
    } else if(!moved) {
        /* Rare jam: nudge the neck by swapping a random upper grain inward. */
        for(int y=2;y>=0;--y) for(int x=0;x<8;++x) {
            if(s.sand[y][x] && allowed(3,y+1) && !s.sand[y+1][3]) {
                s.sand[y][x]=false; s.sand[y+1][3]=true; return;
            }
        }
    }
}

static void update(const xf_input_t *in,uint32_t dt)
{
    if(s.complete){
        s.alarm_ms += dt;
        if(in->activity){
            start();
            return;
        }
        if((s.alarm_ms % 1000U) < dt) buzzer_tone(3000,45);
        return;
    }
    s.elapsed_ms += dt;
    if(s.elapsed_ms>=180){
        s.elapsed_ms=0;
        fall_step();
    }
}

static void render(void)
{
    display_clear();
    for(int y=0;y<8;++y){
        for(int x=0;x<8;++x){
            if(!allowed(x,y)) continue;
            if(s.sand[y][x]) display_set_pixel(x,y,XF_COLOR_YELLOW);
            else display_set_pixel(x,y,xf_rgb(8,12,18));
        }
    }
    if(s.complete && ((s.alarm_ms/250)&1)) {
        display_set_pixel(3,3,XF_COLOR_RED);
        display_set_pixel(4,3,XF_COLOR_RED);
        display_set_pixel(3,4,XF_COLOR_RED);
        display_set_pixel(4,4,XF_COLOR_RED);
    }
}

static bool finished(void){return false;}
static void stop(void){}

const game_module_t *game_hourglass_module(void)
{
    static const game_module_t m={.name="hourglass",.start=start,.update=update,.render=render,.finished=finished,.stop=stop};
    return &m;
}

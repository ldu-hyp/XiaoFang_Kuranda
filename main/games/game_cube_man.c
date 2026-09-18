#include "game_internal.h"

#include <string.h>
#include "esp_random.h"
#include "buzzer.h"
#include "display.h"
#include "storage.h"

typedef enum { FLOOR_NONE=0,FLOOR_NORMAL,FLOOR_FRAGILE,FLOOR_CONVEYOR_L,FLOOR_CONVEYOR_R,FLOOR_SPRING } floor_type_t;

typedef struct {
    uint8_t mask[8];
    floor_type_t type[8];
    xf_point_t player;
    uint32_t elapsed_ms;
    uint16_t score;
    uint16_t high;
    bool over;
} cube_state_t;
static cube_state_t s;

static void make_floor(int y)
{
    s.mask[y]=0;
    s.type[y]=FLOOR_NONE;
    if((esp_random()%100)<35)return;

    int len=3+(esp_random()%3);
    int start=esp_random()%(9-len);
    for(int x=start;x<start+len;++x)s.mask[y]|=1U<<x;

    uint32_t r=esp_random()%10;
    if(r<7)s.type[y]=FLOOR_NORMAL;
    else if(r==7)s.type[y]=FLOOR_FRAGILE;
    else if(r==8)s.type[y]=(esp_random()&1)?FLOOR_CONVEYOR_L:FLOOR_CONVEYOR_R;
    else s.type[y]=FLOOR_SPRING;
}

static esp_err_t start(void)
{
    memset(&s,0,sizeof(s));
    s.player=(xf_point_t){3,5};
    s.mask[6]=0x1c; s.type[6]=FLOOR_NORMAL;
    s.mask[7]=0x7e; s.type[7]=FLOOR_NORMAL;
    s.high=storage_get_high_score(XF_SCORE_CUBE_MAN);
    return ESP_OK;
}

static void update(const xf_input_t *in,uint32_t dt)
{
    if(s.over)return;
    if(in->dir_changed){
        if(in->dir==XF_DIR_LEFT&&s.player.x>0)--s.player.x;
        else if(in->dir==XF_DIR_RIGHT&&s.player.x<7)++s.player.x;
    }

    s.elapsed_ms+=dt;
    uint32_t interval=260U-(s.score<90?s.score*2U:180U);
    if(interval<80)interval=80;
    if(s.elapsed_ms<interval)return;
    s.elapsed_ms=0;

    for(int y=0;y<7;++y){s.mask[y]=s.mask[y+1];s.type[y]=s.type[y+1];}
    make_floor(7);

    int below=s.player.y+1;
    bool supported=below>=0 && below<8 && (s.mask[below]&(1U<<s.player.x));
    if(supported){
        floor_type_t t=s.type[below];
        --s.player.y;
        ++s.score;
        if((s.score%10)==0)buzzer_score();

        if(t==FLOOR_FRAGILE){
            s.mask[below]&=~(1U<<s.player.x);
        }else if(t==FLOOR_CONVEYOR_L&&s.player.x>0){
            --s.player.x;
        }else if(t==FLOOR_CONVEYOR_R&&s.player.x<7){
            ++s.player.x;
        }else if(t==FLOOR_SPRING){
            s.player.y-=2;
        }
    }else{
        ++s.player.y;
    }

    if(s.player.y<0||s.player.y>=8){
        s.over=true;
        if(s.score>s.high)storage_set_high_score(XF_SCORE_CUBE_MAN,s.score);
        buzzer_game_over();
    }
}

static xf_rgb_t floor_color(floor_type_t t)
{
    switch(t){
    case FLOOR_FRAGILE:return xf_rgb(100,100,100);
    case FLOOR_CONVEYOR_L:
    case FLOOR_CONVEYOR_R:return XF_COLOR_GREEN;
    case FLOOR_SPRING:return XF_COLOR_YELLOW;
    case FLOOR_NORMAL:return XF_COLOR_WHITE;
    default:return XF_COLOR_BLACK;
    }
}

static void render(void)
{
    display_clear();
    for(int y=0;y<8;++y)for(int x=0;x<8;++x)
        if(s.mask[y]&(1U<<x))display_set_pixel(x,y,floor_color(s.type[y]));
    if((unsigned)s.player.y<8U)display_set_pixel(s.player.x,s.player.y,s.over?XF_COLOR_RED:XF_COLOR_ORANGE);
}

static bool finished(void){return s.over;}
static void stop(void){}

const game_module_t *game_cube_man_module(void)
{
    static const game_module_t m={.name="cube_man",.start=start,.update=update,.render=render,.finished=finished,.stop=stop};
    return &m;
}

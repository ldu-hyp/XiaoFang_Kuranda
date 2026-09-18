#include "game_internal.h"

#include <string.h>
#include "esp_random.h"
#include "buzzer.h"
#include "display.h"
#include "storage.h"

typedef struct {
    uint8_t rows[8];
    xf_point_t player;
    uint32_t elapsed_ms;
    uint16_t score;
    uint16_t high;
    bool over;
} dodge_state_t;
static dodge_state_t s;

static uint8_t new_obstacle_row(void)
{
    if((esp_random()%3)==0)return 0;
    int gap=(s.score<12)?2:1;
    int start=esp_random()%(9-gap);
    uint8_t row=0xff;
    for(int i=0;i<gap;++i)row&=~(1U<<(start+i));
    return row;
}

static esp_err_t start(void)
{
    memset(&s,0,sizeof(s));
    s.player=(xf_point_t){3,7};
    s.high=storage_get_high_score(XF_SCORE_DODGE);
    return ESP_OK;
}

static void update(const xf_input_t *in,uint32_t dt)
{
    if(s.over)return;
    if(in->dir_changed){
        if(in->dir==XF_DIR_LEFT&&s.player.x>0)--s.player.x;
        else if(in->dir==XF_DIR_RIGHT&&s.player.x<7)++s.player.x;
        else if(in->dir==XF_DIR_UP&&s.player.y>3)--s.player.y;
        else if(in->dir==XF_DIR_DOWN&&s.player.y<7)++s.player.y;
    }

    s.elapsed_ms+=dt;
    uint32_t interval=500U-(s.score<40?s.score*7U:280U);
    if(interval<180)interval=180;
    if(s.elapsed_ms<interval)return;
    s.elapsed_ms=0;

    /* Correct direction: row 0 is the top. Obstacles move DOWN toward player. */
    for(int y=7;y>0;--y)s.rows[y]=s.rows[y-1];
    s.rows[0]=new_obstacle_row();

    if(s.rows[s.player.y]&(1U<<s.player.x)){
        s.over=true;
        if(s.score>s.high)storage_set_high_score(XF_SCORE_DODGE,s.score);
        buzzer_game_over();
    }else{
        ++s.score;
        if((s.score%10)==0)buzzer_score();
    }
}

static void render(void)
{
    display_clear();
    for(int y=0;y<8;++y)for(int x=0;x<8;++x)
        if(s.rows[y]&(1U<<x))display_set_pixel(x,y,XF_COLOR_CYAN);
    display_set_pixel(s.player.x,s.player.y,s.over?XF_COLOR_RED:XF_COLOR_ORANGE);
}

static bool finished(void){return s.over;}
static void stop(void){}

const game_module_t *game_dodge_module(void)
{
    static const game_module_t m={.name="dodge",.start=start,.update=update,.render=render,.finished=finished,.stop=stop};
    return &m;
}

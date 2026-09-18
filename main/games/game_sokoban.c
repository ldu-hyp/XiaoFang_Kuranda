#include "game_internal.h"

#include <string.h>
#include "buzzer.h"
#include "display.h"

#define LEVEL_COUNT 3

static const char *LEVELS[LEVEL_COUNT][8] = {
    {
        "########",
        "#      #",
        "# $.@  #",
        "#      #",
        "#      #",
        "#      #",
        "#      #",
        "########",
    },
    {
        "########",
        "#  .   #",
        "#  $   #",
        "# .$@  #",
        "#      #",
        "#      #",
        "#      #",
        "########",
    },
    {
        "########",
        "# .  . #",
        "# $$   #",
        "#   #  #",
        "# @    #",
        "#      #",
        "#      #",
        "########",
    },
};

typedef struct {
    bool wall[8][8];
    bool goal[8][8];
    bool box[8][8];
    xf_point_t player;
    uint8_t level;
    bool done;
} sokoban_state_t;
static sokoban_state_t s;

static void load_level(uint8_t level)
{
    memset(s.wall,0,sizeof(s.wall));
    memset(s.goal,0,sizeof(s.goal));
    memset(s.box,0,sizeof(s.box));
    s.level=level;

    for(int y=0;y<8;++y){
        for(int x=0;x<8;++x){
            char c=LEVELS[level][y][x];
            if(c=='#')s.wall[y][x]=true;
            else if(c=='.')s.goal[y][x]=true;
            else if(c=='$')s.box[y][x]=true;
            else if(c=='@')s.player=(xf_point_t){x,y};
            else if(c=='*'){s.box[y][x]=true;s.goal[y][x]=true;}
            else if(c=='+'){s.player=(xf_point_t){x,y};s.goal[y][x]=true;}
        }
    }
}

static esp_err_t start(void){memset(&s,0,sizeof(s));load_level(0);return ESP_OK;}

static bool complete(void)
{
    for(int y=0;y<8;++y)for(int x=0;x<8;++x)
        if(s.goal[y][x]&&!s.box[y][x])return false;
    return true;
}

static void update(const xf_input_t *in,uint32_t dt)
{
    (void)dt;
    if(s.done||!in->dir_changed||in->dir==XF_DIR_NONE)return;
    int dx=0,dy=0;
    if(in->dir==XF_DIR_UP)dy=-1;else if(in->dir==XF_DIR_DOWN)dy=1;
    else if(in->dir==XF_DIR_LEFT)dx=-1;else if(in->dir==XF_DIR_RIGHT)dx=1;

    int nx=s.player.x+dx,ny=s.player.y+dy;
    if((unsigned)nx>=8U||(unsigned)ny>=8U||s.wall[ny][nx])return;

    if(s.box[ny][nx]){
        int bx=nx+dx,by=ny+dy;
        if((unsigned)bx>=8U||(unsigned)by>=8U||s.wall[by][bx]||s.box[by][bx])return;
        s.box[ny][nx]=false;
        s.box[by][bx]=true;
    }
    s.player=(xf_point_t){nx,ny};

    if(complete()){
        buzzer_score();
        if(++s.level>=LEVEL_COUNT)s.done=true;
        else load_level(s.level);
    }
}

static void render(void)
{
    display_clear();
    for(int y=0;y<8;++y)for(int x=0;x<8;++x){
        if(s.wall[y][x])display_set_pixel(x,y,xf_rgb(45,45,60));
        else if(s.goal[y][x])display_set_pixel(x,y,XF_COLOR_GREEN);
        if(s.box[y][x])display_set_pixel(x,y,s.goal[y][x]?XF_COLOR_CYAN:XF_COLOR_BLUE);
    }
    display_set_pixel(s.player.x,s.player.y,s.goal[s.player.y][s.player.x]?XF_COLOR_YELLOW:XF_COLOR_RED);
}

static bool finished(void){return s.done;}
static void stop(void){}

const game_module_t *game_sokoban_module(void)
{
    static const game_module_t m={.name="sokoban",.start=start,.update=update,.render=render,.finished=finished,.stop=stop};
    return &m;
}

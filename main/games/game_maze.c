#include "game_internal.h"

#include <string.h>
#include "esp_random.h"
#include "buzzer.h"
#include "display.h"

#define MW 21
#define MH 21
#define MC (MW*MH)

typedef struct {
    bool wall[MH][MW];
    xf_point_t player;
    xf_point_t goal;
    uint16_t wins;
} maze_state_t;
static maze_state_t s;

static void shuffle4(uint8_t d[4])
{
    for(int i=3;i>0;--i){
        int j=esp_random()%(i+1);
        uint8_t t=d[i];d[i]=d[j];d[j]=t;
    }
}

static void generate(void)
{
    memset(&s.wall,1,sizeof(s.wall));
    xf_point_t stack[MC];
    int top=0;
    s.player=(xf_point_t){1,1};
    s.wall[1][1]=false;
    stack[top++]=s.player;

    const int dx[4]={0,2,0,-2};
    const int dy[4]={-2,0,2,0};

    while(top){
        xf_point_t cur=stack[top-1];
        uint8_t dirs[4]={0,1,2,3};
        shuffle4(dirs);
        bool carved=false;
        for(int k=0;k<4;++k){
            int d=dirs[k], nx=cur.x+dx[d], ny=cur.y+dy[d];
            if(nx<=0||ny<=0||nx>=MW-1||ny>=MH-1) continue;
            if(!s.wall[ny][nx]) continue;
            s.wall[cur.y+dy[d]/2][cur.x+dx[d]/2]=false;
            s.wall[ny][nx]=false;
            stack[top++]=(xf_point_t){nx,ny};
            carved=true;
            break;
        }
        if(!carved)--top;
    }

    /* BFS farthest reachable cell: fixes the "random goal can be nearby" issue. */
    int16_t dist[MH][MW];
    for(int y=0;y<MH;++y) for(int x=0;x<MW;++x) dist[y][x]=-1;
    xf_point_t q[MC]; int head=0,tail=0;
    q[tail++]=s.player; dist[s.player.y][s.player.x]=0;
    s.goal=s.player;
    const int sx[4]={0,1,0,-1}, sy[4]={-1,0,1,0};
    while(head<tail){
        xf_point_t p=q[head++];
        if(dist[p.y][p.x] > dist[s.goal.y][s.goal.x]) s.goal=p;
        for(int d=0;d<4;++d){
            int nx=p.x+sx[d],ny=p.y+sy[d];
            if(nx<0||ny<0||nx>=MW||ny>=MH||s.wall[ny][nx]||dist[ny][nx]>=0) continue;
            dist[ny][nx]=dist[p.y][p.x]+1;
            q[tail++]=(xf_point_t){nx,ny};
        }
    }
}

static esp_err_t start(void){memset(&s,0,sizeof(s));generate();return ESP_OK;}

static void update(const xf_input_t *in,uint32_t dt)
{
    (void)dt;
    if(!in->dir_changed||in->dir==XF_DIR_NONE) return;
    int dx=0,dy=0;
    if(in->dir==XF_DIR_UP)dy=-1; else if(in->dir==XF_DIR_DOWN)dy=1;
    else if(in->dir==XF_DIR_LEFT)dx=-1; else if(in->dir==XF_DIR_RIGHT)dx=1;
    int nx=s.player.x+dx,ny=s.player.y+dy;
    if(nx>=0&&ny>=0&&nx<MW&&ny<MH&&!s.wall[ny][nx]){
        s.player=(xf_point_t){nx,ny};
        if(nx==s.goal.x&&ny==s.goal.y){
            ++s.wins;
            buzzer_score();
            generate();
        }
    }
}

static void render(void)
{
    int vx=s.player.x-3,vy=s.player.y-3;
    if(vx<0)vx=0;if(vy<0)vy=0;
    if(vx>MW-8)vx=MW-8;if(vy>MH-8)vy=MH-8;
    display_clear();
    for(int y=0;y<8;++y)for(int x=0;x<8;++x){
        int gx=vx+x,gy=vy+y;
        if(s.wall[gy][gx]) display_set_pixel(x,y,xf_rgb(35,45,70));
    }
    int gx=s.goal.x-vx,gy=s.goal.y-vy;
    if((unsigned)gx<8U&&(unsigned)gy<8U)display_set_pixel(gx,gy,XF_COLOR_GREEN);
    display_set_pixel(s.player.x-vx,s.player.y-vy,XF_COLOR_RED);
}

static bool finished(void){return false;}
static void stop(void){}

const game_module_t *game_maze_module(void)
{
    static const game_module_t m={.name="maze",.start=start,.update=update,.render=render,.finished=finished,.stop=stop};
    return &m;
}

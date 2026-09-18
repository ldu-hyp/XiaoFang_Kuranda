#include "game_internal.h"

#include <string.h>
#include "esp_random.h"
#include "esp_timer.h"
#include "buzzer.h"
#include "display.h"
#include "network.h"
#include "storage.h"
#include "xf_config.h"

enum { PONG_CODE=1, ST_END=0, ST_SEEK=1, ST_JOIN=2, ST_GAME=3 };

typedef enum { ROLE_UNKNOWN=0, ROLE_HOST, ROLE_CLIENT } role_t;

typedef struct {
    role_t role;
    uint8_t self_mac[6];
    uint8_t peer[6];
    uint8_t my_paddle;
    uint8_t peer_paddle;
    int16_t ball_x8;
    int16_t ball_y8;
    int8_t vx8;
    int8_t vy8;
    uint8_t my_score;
    uint8_t peer_score;
    uint32_t elapsed_ms;
    uint32_t seek_ms;
    int64_t started_us;
    int64_t last_rx_us;
    bool paired;
    bool over;
} pong_state_t;
static pong_state_t s;

static bool same_mac(const uint8_t a[6],const uint8_t b[6]){return memcmp(a,b,6)==0;}

static void serve(void)
{
    s.ball_x8=28;s.ball_y8=28;
    int sign=(esp_random()&1)?1:-1;
    s.vx8=sign*2;
    s.vy8=(esp_random()&1)?1:-1;
}

static esp_err_t start(void)
{
    memset(&s,0,sizeof(s));
    s.my_paddle=3;s.peer_paddle=3;
    ESP_RETURN_ON_ERROR(network_init(),"pong","network");
    network_get_mac(s.self_mac);
    s.started_us=esp_timer_get_time();
    s.last_rx_us=s.started_us;
    return ESP_OK;
}

static void send_join(void)
{
    uint8_t p[2]={PONG_CODE,ST_JOIN};
    network_send(s.peer,p,sizeof(p));
}

static void send_state(void)
{
    uint8_t p[8]={PONG_CODE,ST_GAME,s.my_paddle,s.peer_paddle,
                  (uint8_t)(s.ball_x8/8),(uint8_t)(s.ball_y8/8),
                  s.my_score,s.peer_score};
    network_send(s.peer,p,sizeof(p));
}

static void send_paddle(void)
{
    uint8_t p[3]={PONG_CODE,ST_GAME,s.my_paddle};
    network_send(s.peer,p,sizeof(p));
}

static void handle_packet(const xf_net_packet_t *p)
{
    if(p->len<2||p->data[0]!=PONG_CODE||same_mac(p->src,s.self_mac))return;

    if(!s.paired&&p->data[1]==ST_SEEK){
        if(s.role==ROLE_UNKNOWN || (s.role==ROLE_HOST && memcmp(s.self_mac,p->src,6)>0)){
            s.role=ROLE_CLIENT;
            memcpy(s.peer,p->src,6);
            send_join();
            s.paired=true;
            s.last_rx_us=esp_timer_get_time();
            buzzer_menu_enter();
        }
    }else if(!s.paired&&p->data[1]==ST_JOIN&&s.role==ROLE_HOST){
        memcpy(s.peer,p->src,6);
        s.paired=true;
        s.last_rx_us=esp_timer_get_time();
        serve();
        send_state();
        buzzer_menu_enter();
    }else if(s.paired&&same_mac(p->src,s.peer)){
        s.last_rx_us=esp_timer_get_time();
        if(p->data[1]==ST_END){s.over=true;return;}
        if(p->data[1]!=ST_GAME)return;

        if(s.role==ROLE_HOST&&p->len>=3){
            s.peer_paddle=p->data[2];
        }else if(s.role==ROLE_CLIENT&&p->len>=8){
            /* Host world -> client local world: render will mirror X. */
            s.peer_paddle=p->data[2];
            s.ball_x8=p->data[4]*8;
            s.ball_y8=p->data[5]*8;
            s.peer_score=p->data[6]; /* host score from client's view */
            s.my_score=p->data[7];
            if(s.my_score>=5||s.peer_score>=5)s.over=true;
        }
    }
}

static void simulate_host(void)
{
    s.ball_x8+=s.vx8;
    s.ball_y8+=s.vy8;
    if(s.ball_y8<0){s.ball_y8=0;s.vy8=-s.vy8;}
    if(s.ball_y8>56){s.ball_y8=56;s.vy8=-s.vy8;}

    int x=s.ball_x8/8,y=s.ball_y8/8;
    if(s.vx8<0&&x<=0&&(y==s.my_paddle||y==s.my_paddle+1)){
        s.ball_x8=8;s.vx8=-s.vx8;buzzer_tone(3200,20);
    }else if(s.vx8>0&&x>=7&&(y==s.peer_paddle||y==s.peer_paddle+1)){
        s.ball_x8=48;s.vx8=-s.vx8;buzzer_tone(3200,20);
    }

    if(s.ball_x8<0){
        ++s.peer_score;buzzer_score();serve();
    }else if(s.ball_x8>56){
        ++s.my_score;buzzer_score();serve();
    }

    if(s.my_score>=5||s.peer_score>=5){
        uint8_t p[4]={PONG_CODE,ST_END,s.my_score,s.peer_score};
        network_send(s.peer,p,sizeof(p));
        s.over=true;
        uint16_t hi=storage_get_high_score(XF_SCORE_PONG);
        if(s.my_score>hi)storage_set_high_score(XF_SCORE_PONG,s.my_score);
    }
}

static void update(const xf_input_t *in,uint32_t dt)
{
    if(s.over)return;

    if(in->dir_changed){
        if(in->dir==XF_DIR_UP&&s.my_paddle>0)--s.my_paddle;
        else if(in->dir==XF_DIR_DOWN&&s.my_paddle<6)++s.my_paddle;
    }

    xf_net_packet_t p;
    while(network_poll(&p))handle_packet(&p);

    int64_t now=esp_timer_get_time();
    if(!s.paired){
        if(s.role==ROLE_UNKNOWN && now-s.started_us>1000000){
            s.role=ROLE_HOST;
        }
        if(s.role==ROLE_HOST){
            s.seek_ms+=dt;
            if(s.seek_ms>=200){
                s.seek_ms=0;
                uint8_t seek[2]={PONG_CODE,ST_SEEK};
                network_broadcast(seek,sizeof(seek));
            }
        }
        if(now-s.started_us>8000000)s.over=true;
        return;
    }

    if(now-s.last_rx_us>1000000){
        s.over=true;
        return;
    }

    s.elapsed_ms+=dt;
    if(s.elapsed_ms<50)return;
    s.elapsed_ms=0;

    if(s.role==ROLE_HOST){
        simulate_host();
        if(!s.over)send_state();
    }else{
        send_paddle();
    }
}

static void render(void)
{
    display_clear();
    if(!s.paired){
        uint32_t phase=(esp_timer_get_time()/120000)%6;
        display_set_pixel(0,3,XF_COLOR_WHITE);display_set_pixel(0,4,XF_COLOR_WHITE);
        display_set_pixel(7,3,XF_COLOR_WHITE);display_set_pixel(7,4,XF_COLOR_WHITE);
        display_set_pixel(1+phase,3,XF_COLOR_RED);
        return;
    }

    int ballx=s.ball_x8/8,bally=s.ball_y8/8;
    int my=s.my_paddle,peer=s.peer_paddle;
    if(s.role==ROLE_CLIENT){
        /* Fix the original rewrite's coordinate ambiguity: client mirrors host X. */
        ballx=7-ballx;
        int tmp=my; my=s.my_paddle; (void)tmp;
    }
    display_set_pixel(0,my,XF_COLOR_WHITE);display_set_pixel(0,my+1,XF_COLOR_WHITE);
    display_set_pixel(7,peer,XF_COLOR_WHITE);display_set_pixel(7,peer+1,XF_COLOR_WHITE);
    if((unsigned)ballx<8U&&(unsigned)bally<8U)display_set_pixel(ballx,bally,XF_COLOR_RED);
}

static bool finished(void){return s.over;}
static void stop(void)
{
    if(network_is_ready())network_shutdown();
}

const game_module_t *game_pong_module(void)
{
    static const game_module_t m={.name="pong",.start=start,.update=update,.render=render,.finished=finished,.stop=stop};
    return &m;
}

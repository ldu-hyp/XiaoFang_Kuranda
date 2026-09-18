#include "app.h"

#include <math.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "buzzer.h"
#include "display.h"
#include "games/game.h"
#include "imu.h"
#include "input.h"
#include "power.h"
#include "storage.h"
#include "ui.h"
#include "xf_config.h"

static const char *TAG = "app";

typedef enum { APP_MENU, APP_GAME, APP_TEMP } app_state_t;
static app_state_t s_state;
static ui_item_t s_menu;
static xf_settings_t s_settings;
static int64_t s_temp_until;

static bool menu_to_game(ui_item_t item, game_id_t *id)
{
    switch (item) {
    case UI_ITEM_HOURGLASS: *id = GAME_HOURGLASS; return true;
    case UI_ITEM_DICE: *id = GAME_DICE; return true;
    case UI_ITEM_BAGUA: *id = GAME_BAGUA; return true;
    case UI_ITEM_SNAKE: *id = GAME_SNAKE; return true;
    case UI_ITEM_MAZE: *id = GAME_MAZE; return true;
    case UI_ITEM_CUBE_MAN: *id = GAME_CUBE_MAN; return true;
    case UI_ITEM_SOKOBAN: *id = GAME_SOKOBAN; return true;
    case UI_ITEM_DODGE: *id = GAME_DODGE; return true;
    case UI_ITEM_PONG: *id = GAME_PONG; return true;
    default: return false;
    }
}

esp_err_t app_init(void)
{
    ESP_RETURN_ON_ERROR(storage_init(), TAG, "storage");
    storage_load_settings(&s_settings);

    ESP_RETURN_ON_ERROR(display_init(), TAG, "display");
    display_set_brightness(s_settings.brightness);
    ESP_RETURN_ON_ERROR(buzzer_init(), TAG, "buzzer");
    buzzer_set_enabled(s_settings.sound_enabled);
    ESP_RETURN_ON_ERROR(imu_init(), TAG, "imu");
    ESP_RETURN_ON_ERROR(input_init(), TAG, "input");

    power_init((uint32_t)s_settings.sleep_timeout_s * 1000U);
    ui_draw_face_wakeup();

    s_state = APP_MENU;
    s_menu = UI_ITEM_HOURGLASS;
    ui_draw_menu(s_menu);
    display_show();
    ESP_LOGI(TAG, "XiaoFang Kuranda %s ready", XF_FW_VERSION);
    return ESP_OK;
}

static void enter_menu(void)
{
    game_stop();
    s_state = APP_MENU;
    ui_draw_menu(s_menu);
    display_show();
}

static void handle_menu(const xf_input_t *in)
{
    if (in->dir_changed && in->dir == XF_DIR_RIGHT) {
        s_menu = (ui_item_t)((s_menu + 1) % UI_ITEM_COUNT);
        buzzer_menu_move();
        ui_draw_menu(s_menu);
        display_show();
    } else if (in->dir_changed && in->dir == XF_DIR_LEFT) {
        s_menu = (ui_item_t)((s_menu + UI_ITEM_COUNT - 1) % UI_ITEM_COUNT);
        buzzer_menu_move();
        ui_draw_menu(s_menu);
        display_show();
    } else if (in->dir_changed && in->dir == XF_DIR_UP) {
        buzzer_menu_enter();
        game_id_t id;
        if (menu_to_game(s_menu, &id)) {
            if (game_start(id) == ESP_OK) {
                s_state = APP_GAME;
            }
        } else if (s_menu == UI_ITEM_TEMPERATURE) {
            ui_draw_number((int)lroundf(in->temperature_c), XF_COLOR_ORANGE);
            display_show();
            s_temp_until = esp_timer_get_time() + 3000000;
            s_state = APP_TEMP;
        } else if (s_menu == UI_ITEM_SOUND) {
            s_settings.sound_enabled = !s_settings.sound_enabled;
            buzzer_set_enabled(s_settings.sound_enabled);
            storage_save_settings(&s_settings);
            display_fill(s_settings.sound_enabled ? XF_COLOR_GREEN : XF_COLOR_RED);
            display_show();
            vTaskDelay(pdMS_TO_TICKS(180));
            ui_draw_menu(s_menu);
            display_show();
        }
    }
}

void app_run(void)
{
    TickType_t last = xTaskGetTickCount();
    while (true) {
        vTaskDelayUntil(&last, pdMS_TO_TICKS(XF_APP_TICK_MS));

        xf_input_t in;
        if (input_poll(&in) != ESP_OK) {
            continue;
        }
        if (in.activity) power_note_activity();

        if (s_state == APP_MENU) {
            handle_menu(&in);
        } else if (s_state == APP_GAME) {
            if (in.pause) {
                enter_menu();
                continue;
            }
            game_update(&in, XF_APP_TICK_MS);
            game_render();
            display_show();
            if (game_finished()) {
                buzzer_game_over();
                vTaskDelay(pdMS_TO_TICKS(350));
                enter_menu();
            }
        } else if (s_state == APP_TEMP) {
            if (esp_timer_get_time() >= s_temp_until) {
                enter_menu();
            }
        }

        if (in.face_down && s_state != APP_GAME) {
            power_enter_deep_sleep();
        }
        if (power_should_sleep()) {
            power_enter_deep_sleep();
        }
    }
}

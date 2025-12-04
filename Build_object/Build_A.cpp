#include "Build_A.h"

#include "../data/DataCenter.h"
#include "../data/FontCenter.h"
#include "../object/ui.h"

#include <allegro5/allegro_font.h>
#include "../Utils.h"

void Build_A::on_interact() {
    DataCenter* DC = DataCenter::get_instance();
    if(!DC->ui->is_open()) {
        DC->ui->open(this);   // 打開 UI，並指定 target = 這間商店
    } else if (DC->ui->get_target() == this) {
        // 如果目前 UI 就是這間商店的，再按 F 可以關掉
        DC->ui->close();
    } else {
        // 如果 UI 開著但 target 是別人，可以切換 target
        DC->ui->open(this);
    }
}

void Build_A::draw_ui(UI* ui, float x, float y, float w, float h) {
    FontCenter* FC = FontCenter::get_instance();

    ALLEGRO_FONT* font = FC->caviar_dreams[FontSize::SMALL];

    float padding = 20.0f;

    al_draw_text(
        font,
        al_map_rgb(255, 255, 0),
        x + w / 2.0f,
        y + padding,
        ALLEGRO_ALIGN_CENTER,
        "Shop"
    );

    al_draw_text(
        font,
        al_map_rgb(255, 255, 255),
        x + padding,
        y + padding + 40,
        0,
        "- Buy Drink：$50 Restore 20 stamina"
    );
    al_draw_text(
        font,
        al_map_rgb(255, 255, 255),
        x + padding,
        y + padding + 70,
        0,
        "- Buy Bento：$80 Restore 50 stamina"
    );

    // 之後你可以在這裡畫按鈕、ICON 等
}

void Build_A::update_ui(UI* ui) {
    DataCenter* DC = DataCenter::get_instance();

    // 這裡可以簡單示範：按 1 鍵買飲料、按 2 鍵買便當
    if(DC->key_state[ALLEGRO_KEY_1] && !DC->prev_key_state[ALLEGRO_KEY_1]){
        // TODO: 扣錢 加體力
        debug_log("Shop: buy drink\n");
    }
    if(DC->key_state[ALLEGRO_KEY_2] && !DC->prev_key_state[ALLEGRO_KEY_2]){
        // TODO: 扣錢 加體力更多
        debug_log("Shop: buy lunch\n");
    }
}
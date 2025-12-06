#include "Build_A.h"

#include "../data/DataCenter.h"
#include "../data/FontCenter.h"
#include "../object/ui.h"
#include "../object/hero.h"
#include "../object/Phone.h"

#include <allegro5/allegro_font.h>
#include "../Utils.h"

#include <cstdlib> // rand
#include <ctime>   // time

namespace BuildASetting{
    double bento_stamina = 20;
    int bento_cost = 50;
    double drink_stamina = 50;
    int drink_cost = 80;
}

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

    switch(StateA){
        case BuildStateA::Food:{
            if(!in_confirm){
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
            }else{
                const char* item_name = (pending_item == 1 ? "Drink" : "Bento");
                al_draw_text(
                    font,
                    al_map_rgb(255, 255, 0),
                    x + w / 2.0f,
                    y + padding,
                    ALLEGRO_ALIGN_CENTER,
                    "Confirm Purchase"
                );
                char buf[100];
                sprintf(buf, "Buy %s? Press %d again to confirm, ESC to cancel.", item_name, pending_item);
                al_draw_text(
                    font,
                    al_map_rgb(255, 255, 255),
                    x + padding,
                    y + padding + 60,
                    0,
                    buf
                );
            }
            break;
        }
        case BuildStateA::Nothing:
        default:{
            al_draw_text(
                font,
                al_map_rgb(255, 255, 0),
                x + w / 2.0f,
                y + padding,
                ALLEGRO_ALIGN_CENTER,
                "Nothing here now"
            );
            break;
        }
    }
}

void Build_A::update_ui(UI* ui) {
    DataCenter* DC = DataCenter::get_instance();

    if(StateA != BuildStateA::Food){
        // 沒食物時，UI 只能看，不能做事
        return;
    }

    // 讀 key edge（你原本就有 key_state / prev_key_state）
    bool key1_pressed = DC->key_state[ALLEGRO_KEY_1] && !DC->prev_key_state[ALLEGRO_KEY_1];
    bool key2_pressed = DC->key_state[ALLEGRO_KEY_2] && !DC->prev_key_state[ALLEGRO_KEY_2];
    bool esc_pressed  = DC->key_state[ALLEGRO_KEY_ESCAPE] && !DC->prev_key_state[ALLEGRO_KEY_ESCAPE];

    if(!in_confirm){
        // 第一階段：選擇要買哪一項
        if(key1_pressed){
            in_confirm = true;
            pending_item = 1;
            debug_log("Shop: choose Drink, go to confirm page.\n");
        } else if(key2_pressed){
            in_confirm = true;
            pending_item = 2;
            debug_log("Shop: choose Bento, go to confirm page.\n");
        }
    } else {
        // 第二階段：確認 or 取消
        if(esc_pressed){
            // 取消購買，回到菜單畫面
            in_confirm = false;
            pending_item = 0;
            debug_log("Shop: cancel purchase.\n");
            return;
        }

        // 再按一次同一個數字 → 確認購買
        if(pending_item == 1 && key1_pressed){
            debug_log("Shop: confirm buy Drink.\n");
            DC->hero->add_stamina(BuildASetting::drink_stamina);
            DC->hero->reduce_deposit(BuildASetting::drink_cost);

            // 買完這次後，食物賣完 → 回到 Nothing
            StateA = BuildStateA::Nothing;
            in_confirm = false;
            pending_item = 0;

            DC->ui->close();
        } else if(pending_item == 2 && key2_pressed){
            debug_log("Shop: confirm buy Bento.\n");
            DC->hero->add_stamina(BuildASetting::bento_stamina);
            DC->hero->reduce_deposit(BuildASetting::bento_cost);

            StateA = BuildStateA::Nothing;
            in_confirm = false;
            pending_item = 0;

            DC->ui->close();
        }
    }
}

void Build_A::child_update(){
    DataCenter* DC = DataCenter::get_instance();
    if(StateA == BuildStateA::Nothing){

        if(DC->ui->is_open()) return;

        frames_passed += 1;
        if(frames_passed < interval_frames) return;
        frames_passed = 0;

        float r = std::rand() / (float)RAND_MAX;
        if(r < cur_prob){
            debug_log("Lucky event TRIGGERED! p = %f\n", cur_prob);

            // 中獎後機率重設
            cur_prob = base_prob;
            StateA = BuildStateA::Food;
            DC->phone->add_notification(
                "1145141919810 WOW",
                "可以來買午餐了",
                "教院的人已經瘋狂從山上跑下來搶午餐了"
            );
        } else {
            cur_prob += prob_step;
            if(cur_prob > max_prob) cur_prob = max_prob;

            debug_log("Lucky event MISSED! new p = %f\n", cur_prob);
        }
    }else{
        
    }
}

void Build_A::child_init(){

}   
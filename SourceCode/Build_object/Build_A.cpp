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
    double bento_stamina = 50;
    int bento_cost = 80;
    double drink_stamina = 20;
    int drink_cost = 50;
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

    ALLEGRO_FONT* font = FC->NotoSansCJK[FontSize::SMALL];

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
                    "-(1) 花費 $50 買飲料，恢復 20 飽食度"
                );
                al_draw_text(
                    font,
                    al_map_rgb(255, 255, 255),
                    x + padding,
                    y + padding + 70,
                    0,
                    "-(2) 花費 $80 買便當，恢復 50 飽食度"
                );
            }else{
                const char* item_name = (pending_item == 1 ? "飲料" : "便當");
                al_draw_text(
                    font,
                    al_map_rgb(255, 255, 0),
                    x + w / 2.0f,
                    y + padding,
                    ALLEGRO_ALIGN_CENTER,
                    "Confirm Purchase"
                );
                char buf[100];
                sprintf(buf, "確定購買 %s?", item_name);
                al_draw_text(
                    font,
                    al_map_rgb(255, 255, 255),
                    x + padding,
                    y + padding + 60,
                    0,
                    buf
                );
                al_draw_text(
                    font,
                    al_map_rgb(255, 255, 255),
                    x + padding,
                    y + padding + 90,
                    0,
                    "按下E確認，Q 取消"
                );
            }

            // 顯示錢不夠 / 其他提示
            if (ui_message_timer > 0 && !ui_message.empty()) {
                al_draw_text(
                    font,
                    al_map_rgb(255, 80, 80), // 紅色提示
                    x + padding,
                    y + padding + 120,
                    0,
                    ui_message.c_str()
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

    if (StateA != BuildStateA::Food) {
        // 沒食物時，UI 只能看，不能做事
        return;
    }
    // UI 提示倒數
    if (ui_message_timer > 0) {
        ui_message_timer--;
        if (ui_message_timer <= 0) ui_message.clear();
    }

    // 讀 key edge
    bool key1_pressed = DC->key_state[ALLEGRO_KEY_1] && !DC->prev_key_state[ALLEGRO_KEY_1];
    bool key2_pressed = DC->key_state[ALLEGRO_KEY_2] && !DC->prev_key_state[ALLEGRO_KEY_2];
    bool esc_pressed  = DC->key_state[ALLEGRO_KEY_Q] && !DC->prev_key_state[ALLEGRO_KEY_Q];
    bool keyE_pressed = DC->key_state[ALLEGRO_KEY_E] && !DC->prev_key_state[ALLEGRO_KEY_E];

    if (!in_confirm) {
        // ————————————————
        // 第一階段：選擇買什麼
        // ————————————————
        if (key1_pressed) {
            in_confirm = true;
            pending_item = 1;
            debug_log("Shop: choose Drink, go to confirm.\n");
        }
        else if (key2_pressed) {
            in_confirm = true;
            pending_item = 2;
            debug_log("Shop: choose Bento, go to confirm.\n");
        }
    }
    else {
        // ————————————————
        // 第二階段：確認 / 取消
        // ————————————————
        if (esc_pressed) {
            // 取消 → 回到主菜單
            in_confirm = false;
            pending_item = 0;
            debug_log("Shop: cancel purchase.\n");
            return;
        }

        // ⭐ 按 E 確認購買（統一按鍵）
        if (keyE_pressed) {
            int cost = 0;
            double stamina = 0;
            const char* item_name = "";
            
            if (pending_item == 1) {
                // Drink
                cost = BuildASetting::drink_cost;
                stamina = BuildASetting::drink_stamina;
                item_name = "飲料";
            }
            else if (pending_item == 2) {
                // Bento
                cost = BuildASetting::bento_cost;
                stamina = BuildASetting::bento_stamina;
                item_name = "便當";
            }
            
            // 檢查是否有足夠的錢
            if (DC->hero->can_afford(cost)) {
                debug_log("Shop: confirm buy %s.\n", item_name);
                DC->hero->add_stamina(stamina);
                DC->hero->reduce_deposit(cost);
                
                // 結帳後商品售罄
                StateA = BuildStateA::Nothing;
                in_confirm = false;
                pending_item = 0;
                
                DC->ui->close();
            } else {
                debug_log("Shop: not enough money to buy %s.\n", item_name);

                // 留在確認頁面，顯示提示（2 秒）
                char msg[100];
                sprintf(msg, "錢不夠！需要 $%d，目前 $%d", cost, (int)DC->hero->get_deposit());
                ui_message = msg;
                ui_message_timer = 120; // 假設 60 FPS -> 2 秒
            }
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
            debug_log("Shiao Chi Bu start to sell some lunch\n");
            // 中獎後機率重設
            cur_prob = base_prob;
            StateA = BuildStateA::Food;
            DC->phone->add_notification(
                "小吃部",
                "午餐已開始販售",
                "教院的人已經瘋狂從山上跑下來搶午餐了"
            );
        } else {
            cur_prob += prob_step;
            if(cur_prob > max_prob) cur_prob = max_prob;
        }
    }else{
        
    }
}

void Build_A::child_init(){

}   
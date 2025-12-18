#include "Build_C.h"

#include "../data/DataCenter.h"
#include "../data/FontCenter.h"
#include "../data/ImageCenter.h"
#include "../object/ui.h"
#include "../object/hero.h"
#include "../object/Phone.h"

#include <allegro5/allegro_font.h>
#include "../Utils.h"

#include <cstdlib> // rand
#include <ctime>   // time

namespace BuildCSetting{
    double bento_stamina = 100;
    int bento_cost = 50;
    double drink_stamina = 40;
    int drink_cost = 0;

    static constexpr const char* img_shop  = "./assets/image/A_ui/bread.jpg";     // 餐廳
    static constexpr const char* img_bento = "./assets/image/A_ui/monster.jpg";    // 飲料
    static constexpr const char* img_drink = "./assets/image/A_ui/walter_smile.jpg";    // 便當
}

void Build_C::on_interact() {
    if (!spawned) return; // ✅ 沒出現不能互動

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

void Build_C::draw_ui(UI* ui, float x, float y, float w, float h) {
    if (!spawned) return; // ✅ 沒出現就不畫 UI（理論上也不會被打開，但保險）
    FontCenter* FC = FontCenter::get_instance();

    ALLEGRO_FONT* font = FC->NotoSansCJK[FontSize::SMALL];

    float padding = 20.0f;

    switch(StateA){
        case BuildStateC::Food:{
            if(!in_confirm){
                al_draw_text(
                    font,
                    al_map_rgb(255, 255, 0),
                    x + w / 2.0f,
                    y + padding,
                    ALLEGRO_ALIGN_CENTER,
                    "神秘的黑市"
                );
                al_draw_text(font, al_map_rgb(255,255,255),
                            x + padding, y + padding + 40, 0,
                            "-(1) 花費 $50 買冰毒，恢復 100 飽食度");

                // 只有魔爪有貨才顯示 (2)
                if (claw_available) {
                    al_draw_text(font, al_map_rgb(255,255,255),
                                x + padding, y + padding + 70, 0,
                                "-(2) 領取魔爪，恢復 40 飽食度");
                } else {
                    al_draw_text(font, al_map_rgb(200,200,200),
                                x + padding, y + padding + 70, 0,
                                "（魔爪缺貨）");
                }
            }else{
                const char* item_name = (pending_item == 1 ? "冰毒" : "魔爪");
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
        case BuildStateC::Nothing:
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

    ImageCenter* IC = ImageCenter::get_instance();

    const char* img_path = BuildCSetting::img_shop;

    if (StateA == BuildStateC::Food && in_confirm) {
        // 確認頁：依 pending_item
        if (pending_item == 1)
            img_path = BuildCSetting::img_drink;
        else if (pending_item == 2)
            img_path = BuildCSetting::img_bento;
    }

    ALLEGRO_BITMAP* ui_img = IC->get(img_path);
    if (ui_img) {
        float padding_img = 10.0f;
        float reserve_bottom = 40.0f; // 留給錯誤提示

        float img_x1 = x + padding;
        float img_x2 = x + w - padding;
        float img_y1 = y + h * 0.55f;
        float img_y2 = y + h - reserve_bottom;

        if (img_y2 > img_y1) {
            int bw = al_get_bitmap_width(ui_img);
            int bh = al_get_bitmap_height(ui_img);

            al_draw_scaled_bitmap(
                ui_img,
                0, 0, bw, bh,
                img_x1, img_y1,
                img_x2 - img_x1,
                img_y2 - img_y1,
                0
            );
        }
    }
}

void Build_C::update_ui(UI* ui) {
    if (!spawned) return; // ✅ 沒出現就不處理按鍵
    
    DataCenter* DC = DataCenter::get_instance();

    if (StateA != BuildStateC::Food) {
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
            pending_item = 1; // 冰毒永遠可買
        }
        else if (key2_pressed) {
            if (claw_available) {
                in_confirm = true;
                pending_item = 2; // 魔爪
            } else {
                ui_message = "魔爪缺貨！";
                ui_message_timer = 120;
            }
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
            
            if (pending_item == 1) { // 冰毒
                cost = BuildCSetting::bento_cost;
                stamina = BuildCSetting::bento_stamina;
                item_name = "冰毒";
            }
            else if (pending_item == 2) { // 魔爪
                cost = BuildCSetting::drink_cost;
                stamina = BuildCSetting::drink_stamina;
                item_name = "魔爪";
            }
            
            // 檢查是否有足夠的錢
            if (DC->hero->can_afford(cost)) {
                DC->hero->add_stamina(stamina);
                DC->hero->reduce_deposit(cost);
                DC->hero->add_score(10);

                if (pending_item == 2) {
                    claw_available = false;   // ✅ 魔爪買完就沒了
                }

                // ✅ 你要不要讓店整間「買一次就沒」？
                // 如果你希望店還在（冰毒一直都在）→ 不要把 StateA 變 Nothing，也不要關 UI
                // 下面兩行請刪掉/註解：
                // StateA = BuildStateC::Nothing;
                // DC->ui->close();

                in_confirm = false;
                pending_item = 0;
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

void Build_C::child_update() {
    DataCenter* DC = DataCenter::get_instance();

    // UI 開著時不切狀態，避免出現到一半消失造成怪狀況
    if (DC->ui->is_open() && DC->ui->get_target() == this) return;

    // ====== 若目前沒出現：定期 roll 是否出現 ======
    if (!spawned) {
        spawn_check_cnt++;
        if (spawn_check_cnt < spawn_check_frames) return;
        spawn_check_cnt = 0;

        float r = std::rand() / (float)RAND_MAX;
        if (r < spawn_prob) {
            spawned = true;
            stay_cnt = stay_frames;

            // 出現時可以交易
            StateA = BuildStateC::Food;

            // ✅ 魔爪刷機率：每次店出現時 roll 一次
            float rr = std::rand() / (float)RAND_MAX;
            claw_available = (rr < claw_prob);
        }
        return;
    }

    // ====== 若目前出現中：倒數停留時間，到期就消失 ======
    if (spawned) {
        if (stay_cnt > 0) {
            stay_cnt--;
        }

        if (stay_cnt <= 0) {
            spawned = false;

            in_confirm = false;
            pending_item = 0;
            ui_message.clear();
            ui_message_timer = 0;

            StateA = BuildStateC::Nothing;

            claw_available = false; // ✅ 消失就清空
        }
    }
}

void Build_C::child_init() {
    spawned = false;
    spawn_check_cnt = 0;
    stay_cnt = 0;

    StateA = BuildStateC::Nothing;
    in_confirm = false;
    pending_item = 0;
    ui_message.clear();
    ui_message_timer = 0;
}

void Build_C::draw() {
    if (!spawned) return;   // ✅ 沒出現就不畫
    Build::draw();          // ✅ 出現就照原本建築畫法
}
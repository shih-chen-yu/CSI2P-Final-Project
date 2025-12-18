#include "Build_B.h"

#include "../data/DataCenter.h"
#include "../data/FontCenter.h"
#include "../data/ImageCenter.h"
#include "../object/ui.h"
#include "../object/hero.h"
#include "../object/Phone.h"
#include "../Utils.h"

#include <allegro5/allegro_font.h>

#include <cstdlib>
#include <cstdio>
#include <string>

namespace BuildBSetting{
    double bento_stamina = 100;
    int bento_cost = 500;
    double drink_stamina = 40;
    int drink_cost = 50;

    static constexpr const char* img_shop  = "./assets/image/A_ui/bread.jpg";
    static constexpr const char* img_bento = "./assets/image/A_ui/monster.jpg";
    static constexpr const char* img_drink = "./assets/image/A_ui/meth.jpg";

    // 手機顯示用
    static constexpr const char* phone_from = "奇怪的商店";
}

// 每棟 building 唯一 key（避免多棟互相覆蓋）
static std::string make_buildB_phone_key(const Build_B* self) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "BUILD_B_%p", (void*)self);
    return std::string(buf);
}

// ===== Phone helpers =====
void Build_B::notify_food_spawn() {
    DataCenter* DC = DataCenter::get_instance();
    if (!DC || !DC->phone) return;

    if (phone_key.empty()) phone_key = make_buildB_phone_key(this);

    char msg[128];
    std::snprintf(msg, sizeof(msg),
        "進貨了！目前共有 %d 份",
        (food_amount > 0 ? food_amount : 1)
    );

    DC->phone->upsert_food_status(
        phone_key.c_str(),
        BuildBSetting::phone_from,
        "商品已上架",
        msg
    );
}

void Build_B::notify_food_taken(const char* who) {
    DataCenter* DC = DataCenter::get_instance();
    if (!DC || !DC->phone) return;

    if (phone_key.empty()) phone_key = make_buildB_phone_key(this);

    // ⚠️ 只有「真的被拿走」才會呼叫這個函式
    if (!has_food()) {
        DC->phone->upsert_food_status(
            phone_key.c_str(),
            BuildBSetting::phone_from,
            "已售罄",
            "目前沒有剩餘商品"
        );
        return;
    }

    char title[64];
    char msg[128];
    std::snprintf(title, sizeof(title), "%s拿走了一份", who ? who : "有人");
    std::snprintf(msg, sizeof(msg), "剩下 %d 份", (food_amount > 0 ? food_amount : 1));

    DC->phone->upsert_food_status(
        phone_key.c_str(),
        BuildBSetting::phone_from,
        title,
        msg
    );
}

// 同步現在狀態到手機（⚠️不要亂叫成「被搶走」！）
void Build_B::sync_phone() {
    DataCenter* DC = DataCenter::get_instance();
    if (!DC || !DC->phone) return;

    if (phone_key.empty()) phone_key = make_buildB_phone_key(this);

    if (!has_food()) {
        // 沒貨 → 顯示售罄（或你想 clear 也行）
        DC->phone->upsert_food_status(
            phone_key.c_str(),
            BuildBSetting::phone_from,
            "已售罄",
            "目前沒有剩餘商品"
        );
        return;
    }

    // 有貨 → 顯示「目前有幾份」，不要用「被搶走」
    char msg[128];
    std::snprintf(msg, sizeof(msg), "目前剩下 %d 份", (food_amount > 0 ? food_amount : 1));

    DC->phone->upsert_food_status(
        phone_key.c_str(),
        BuildBSetting::phone_from,
        "商品供應中",
        msg
    );
}

// ===== core state =====
bool Build_B::has_food() const {
    if (food_amount > 0) return true;
    return StateB == BuildStateB::Food;
}

void Build_B::set_stateB(BuildStateB s) {
    StateB = s;
    sync_phone();
}

void Build_B::set_food_amount(int n) {
    food_amount = n;

    if (food_amount <= 0) {
        food_amount = 0;
        StateB = BuildStateB::Nothing;
        sync_phone();
        return;
    }

    // 有份數就一定是 Food
    StateB = BuildStateB::Food;
    notify_food_spawn();   // ✅ 只有上架時才推播「進貨了」
    sync_phone();          // ✅ 顯示目前剩幾份
}

// 有人（玩家 / NPC）拿走一份：扣份數 + 手機顯示「誰拿走」+ 剩下幾份
bool Build_B::take_food(int amount, const char* who) {
    if (amount <= 0) return false;

    if (!has_food()) return false;

    if (food_amount > 0) {
        food_amount -= amount;
        if (food_amount <= 0) {
            food_amount = 0;
            StateB = BuildStateB::Nothing;
        } else {
            StateB = BuildStateB::Food;
        }

        notify_food_taken(who);
        return true;
    }

    // 沒有追蹤份數時：直接清掉 Food
    if (StateB == BuildStateB::Food) {
        StateB = BuildStateB::Nothing;
        notify_food_taken(who);
        return true;
    }

    return false;
}

// ===== UI =====
void Build_B::on_interact() {
    DataCenter* DC = DataCenter::get_instance();
    if(!DC->ui->is_open()) {
        DC->ui->open(this);
    } else if (DC->ui->get_target() == this) {
        DC->ui->close();
    } else {
        DC->ui->open(this);
    }
}

void Build_B::draw_ui(UI* ui, float x, float y, float w, float h) {
    FontCenter* FC = FontCenter::get_instance();
    ALLEGRO_FONT* font = FC->NotoSansCJK[FontSize::SMALL];

    float padding = 20.0f;

    switch(StateB){
        case BuildStateB::Food:{
            if(!in_confirm){
                al_draw_text(font, al_map_rgb(255,255,0),
                    x + w/2.0f, y + padding, ALLEGRO_ALIGN_CENTER, "Shop");

                // 顯示剩幾份（你要）
                char leftbuf[64];
                std::snprintf(leftbuf, sizeof(leftbuf), "剩餘份數：%d", (food_amount > 0 ? food_amount : 1));
                al_draw_text(font, al_map_rgb(200,200,200),
                    x + padding, y + padding + 25, 0, leftbuf);

                al_draw_text(font, al_map_rgb(255,255,255),
                    x + padding, y + padding + 55, 0,
                    "-(1) 花費 $500 買冰毒，恢復 100 飽食度");

                al_draw_text(font, al_map_rgb(255,255,255),
                    x + padding, y + padding + 85, 0,
                    "-(2) 花費 $50 買魔爪，恢復 40 飽食度");
            } else {
                const char* item_name = (pending_item == 1 ? "冰毒" : "魔爪");
                al_draw_text(font, al_map_rgb(255,255,0),
                    x + w/2.0f, y + padding, ALLEGRO_ALIGN_CENTER, "Confirm Purchase");

                char buf[128];
                std::snprintf(buf, sizeof(buf), "確定購買 %s?", item_name);
                al_draw_text(font, al_map_rgb(255,255,255),
                    x + padding, y + padding + 60, 0, buf);

                al_draw_text(font, al_map_rgb(255,255,255),
                    x + padding, y + padding + 90, 0, "按下E確認，Q 取消");
            }

            if (ui_message_timer > 0 && !ui_message.empty()) {
                al_draw_text(font, al_map_rgb(255,80,80),
                    x + padding, y + padding + 120, 0, ui_message.c_str());
            }
            break;
        }
        case BuildStateB::Nothing:
        default:{
            al_draw_text(font, al_map_rgb(255,255,0),
                x + w/2.0f, y + padding, ALLEGRO_ALIGN_CENTER, "Nothing here now");
            break;
        }
    }

    ImageCenter* IC = ImageCenter::get_instance();
    const char* img_path = BuildBSetting::img_shop;

    if (StateB == BuildStateB::Food && in_confirm) {
        if (pending_item == 1) img_path = BuildBSetting::img_drink;
        else if (pending_item == 2) img_path = BuildBSetting::img_bento;
    }

    ALLEGRO_BITMAP* ui_img = IC->get(img_path);
    if (ui_img) {
        float reserve_bottom = 40.0f;
        float img_x1 = x + padding;
        float img_x2 = x + w - padding;
        float img_y1 = y + h * 0.55f;
        float img_y2 = y + h - reserve_bottom;

        if (img_y2 > img_y1) {
            int bw = al_get_bitmap_width(ui_img);
            int bh = al_get_bitmap_height(ui_img);

            al_draw_scaled_bitmap(ui_img,
                0,0,bw,bh,
                img_x1, img_y1,
                img_x2 - img_x1, img_y2 - img_y1,
                0);
        }
    }
}

void Build_B::update_ui(UI* ui) {
    DataCenter* DC = DataCenter::get_instance();

    if (StateB != BuildStateB::Food) return;

    if (ui_message_timer > 0) {
        ui_message_timer--;
        if (ui_message_timer <= 0) ui_message.clear();
    }

    bool key1_pressed = DC->key_state[ALLEGRO_KEY_1] && !DC->prev_key_state[ALLEGRO_KEY_1];
    bool key2_pressed = DC->key_state[ALLEGRO_KEY_2] && !DC->prev_key_state[ALLEGRO_KEY_2];
    bool esc_pressed  = DC->key_state[ALLEGRO_KEY_Q] && !DC->prev_key_state[ALLEGRO_KEY_Q];
    bool keyE_pressed = DC->key_state[ALLEGRO_KEY_E] && !DC->prev_key_state[ALLEGRO_KEY_E];

    if (!in_confirm) {
        if (key1_pressed) { in_confirm = true; pending_item = 1; }
        else if (key2_pressed) { in_confirm = true; pending_item = 2; }
    } else {
        if (esc_pressed) {
            in_confirm = false;
            pending_item = 0;
            return;
        }

        if (keyE_pressed) {
            int cost = 0;
            double stamina = 0;
            const char* item_name = "";

            if (pending_item == 1) { cost = BuildBSetting::bento_cost; stamina = BuildBSetting::bento_stamina; item_name = "冰毒"; }
            else if (pending_item == 2) { cost = BuildBSetting::drink_cost; stamina = BuildBSetting::drink_stamina; item_name = "魔爪"; }

            if (DC->hero->can_afford(cost)) {
                DC->hero->add_stamina(stamina);
                DC->hero->reduce_deposit(cost);

                // ✅ 用 take_food 扣 1 份 + 手機顯示 "玩家拿走" + 剩幾份
                take_food(1, "你");

                in_confirm = false;
                pending_item = 0;
                DC->ui->close();
            } else {
                char msg[100];
                std::snprintf(msg, sizeof(msg), "錢不夠！需要 $%d，目前 $%d", cost, (int)DC->hero->get_deposit());
                ui_message = msg;
                ui_message_timer = 120;
            }
        }
    }
}

// ===== spawn resource =====
void Build_B::child_update(){
    DataCenter* DC = DataCenter::get_instance();

    if(StateB == BuildStateB::Nothing){
        if(DC->ui->is_open()) return;

        frames_passed += 1;
        if(frames_passed < interval_frames) return;
        frames_passed = 0;

        float r = std::rand() / (float)RAND_MAX;
        if(r < cur_prob){
            cur_prob = base_prob;

            // ✅ 一次出 3 份（不會 NPC 一來就清空）
            set_food_amount(3);

            DC->phone->add_notification(
                BuildBSetting::phone_from,
                "進貨了一些奇怪的東西",
                "歡迎各位有興趣過來參考一下"
            );
        } else {
            cur_prob += prob_step;
            if(cur_prob > max_prob) cur_prob = max_prob;
        }
    }
}

void Build_B::child_init(){
    phone_key.clear();
    StateB = BuildStateB::Nothing;
    food_amount = 0;
    targeted_by_monster = false;
}

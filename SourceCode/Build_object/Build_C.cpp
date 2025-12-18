#include "Build_C.h"

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

// ==================== Settings（你改這裡就好） ====================
namespace BuildCSetting {
    // 你要賣的兩個品項
    double item1_stamina = 50;
    int    item1_cost    = 80;
    const char* item1_name = "品項1";

    double item2_stamina = 20;
    int    item2_cost    = 50;
    const char* item2_name = "品項2";

    // UI 圖片
    static constexpr const char* img_shop  = "./assets/image/C_ui/shop.jpg";
    static constexpr const char* img_item1 = "./assets/image/C_ui/item1.png";
    static constexpr const char* img_item2 = "./assets/image/C_ui/item2.png";

    // 手機顯示用
    static constexpr const char* phone_from = "C商店";
}
// =================================================================

// 每棟 building 唯一 key（避免多棟互相覆蓋）
static std::string make_buildC_phone_key(const Build_C* self) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "BUILD_C_%p", (void*)self);
    return std::string(buf);
}

// ==================== Phone helpers ====================
void Build_C::notify_food_spawn() {
    DataCenter* DC = DataCenter::get_instance();
    if (!DC || !DC->phone) return;

    if (phone_key.empty()) phone_key = make_buildC_phone_key(this);

    char msg[128];
    std::snprintf(msg, sizeof(msg),
        "上架了！目前共有 %d 份",
        (food_amount > 0 ? food_amount : 1)
    );

    DC->phone->upsert_food_status(
        phone_key.c_str(),
        BuildCSetting::phone_from,
        "商品已上架",
        msg
    );
}

void Build_C::notify_food_taken(const char* who) {
    DataCenter* DC = DataCenter::get_instance();
    if (!DC || !DC->phone) return;

    if (phone_key.empty()) phone_key = make_buildC_phone_key(this);

    if (!has_food()) {
        DC->phone->upsert_food_status(
            phone_key.c_str(),
            BuildCSetting::phone_from,
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
        BuildCSetting::phone_from,
        title,
        msg
    );
}

void Build_C::sync_phone() {
    DataCenter* DC = DataCenter::get_instance();
    if (!DC || !DC->phone) return;

    if (phone_key.empty()) phone_key = make_buildC_phone_key(this);

    if (!has_food()) {
        DC->phone->upsert_food_status(
            phone_key.c_str(),
            BuildCSetting::phone_from,
            "已售罄",
            "目前沒有剩餘商品"
        );
        return;
    }

    char msg[128];
    std::snprintf(msg, sizeof(msg),
        "目前剩下 %d 份",
        (food_amount > 0 ? food_amount : 1)
    );

    DC->phone->upsert_food_status(
        phone_key.c_str(),
        BuildCSetting::phone_from,
        "商品供應中",
        msg
    );
}
// ======================================================

// ==================== core state ====================
bool Build_C::has_food() const {
    if (food_amount > 0) return true;
    return StateC == BuildStateC::Food;
}

void Build_C::set_stateC(BuildStateC s) {
    StateC = s;
    sync_phone();
}

void Build_C::set_food_amount(int n) {
    food_amount = n;

    if (food_amount <= 0) {
        food_amount = 0;
        StateC = BuildStateC::Nothing;
        sync_phone();
        return;
    }

    StateC = BuildStateC::Food;
    notify_food_spawn();   // ✅ 只有生成/上架時才推播
    sync_phone();          // ✅ 顯示目前剩幾份
}

bool Build_C::take_food(int amount, const char* who) {
    if (amount <= 0) return false;
    if (!has_food()) return false;

    if (food_amount > 0) {
        food_amount -= amount;
        if (food_amount <= 0) {
            food_amount = 0;
            StateC = BuildStateC::Nothing;
        } else {
            StateC = BuildStateC::Food;
        }
        notify_food_taken(who);
        return true;
    }

    // 沒有追蹤份數時：直接清掉 Food
    if (StateC == BuildStateC::Food) {
        StateC = BuildStateC::Nothing;
        notify_food_taken(who);
        return true;
    }

    return false;
}
// ======================================================

// ==================== UI ====================
void Build_C::on_interact() {
    DataCenter* DC = DataCenter::get_instance();
    if (!DC->ui->is_open()) {
        DC->ui->open(this);
    } else if (DC->ui->get_target() == this) {
        DC->ui->close();
    } else {
        DC->ui->open(this);
    }
}

void Build_C::draw_ui(UI* ui, float x, float y, float w, float h) {
    FontCenter* FC = FontCenter::get_instance();
    ALLEGRO_FONT* font = FC->NotoSansCJK[FontSize::SMALL];

    float padding = 20.0f;

    switch (StateC) {
        case BuildStateC::Food: {
            if (!in_confirm) {
                // 標題
                al_draw_text(font, al_map_rgb(255,255,0),
                    x + w/2.0f, y + padding, ALLEGRO_ALIGN_CENTER, BuildCSetting::phone_from);

                // 顯示剩餘份數
                char leftbuf[64];
                std::snprintf(leftbuf, sizeof(leftbuf), "剩餘份數：%d", (food_amount > 0 ? food_amount : 1));
                al_draw_text(font, al_map_rgb(200,200,200),
                    x + padding, y + padding + 25, 0, leftbuf);

                // 選單
                char line1[128], line2[128];
                std::snprintf(line1, sizeof(line1),
                    "-(1) 花費 $%d 買%s，恢復 %.0f 飽食度",
                    BuildCSetting::item1_cost, BuildCSetting::item1_name, BuildCSetting::item1_stamina);
                std::snprintf(line2, sizeof(line2),
                    "-(2) 花費 $%d 買%s，恢復 %.0f 飽食度",
                    BuildCSetting::item2_cost, BuildCSetting::item2_name, BuildCSetting::item2_stamina);

                al_draw_text(font, al_map_rgb(255,255,255), x + padding, y + padding + 55, 0, line1);
                al_draw_text(font, al_map_rgb(255,255,255), x + padding, y + padding + 85, 0, line2);
            } else {
                const char* item_name = (pending_item == 1 ? BuildCSetting::item1_name : BuildCSetting::item2_name);

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
        case BuildStateC::Nothing:
        default: {
            al_draw_text(font, al_map_rgb(255,255,0),
                x + w/2.0f, y + padding, ALLEGRO_ALIGN_CENTER, "Nothing here now");
            break;
        }
    }

    // 圖片區
    ImageCenter* IC = ImageCenter::get_instance();
    const char* img_path = BuildCSetting::img_shop;

    if (StateC == BuildStateC::Food && in_confirm) {
        if (pending_item == 1) img_path = BuildCSetting::img_item1;
        else if (pending_item == 2) img_path = BuildCSetting::img_item2;
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

void Build_C::update_ui(UI* ui) {
    DataCenter* DC = DataCenter::get_instance();
    if (StateC != BuildStateC::Food) return;

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
            double stamina = 0.0;
            const char* item_name = "";

            if (pending_item == 1) { cost = BuildCSetting::item1_cost; stamina = BuildCSetting::item1_stamina; item_name = BuildCSetting::item1_name; }
            else if (pending_item == 2) { cost = BuildCSetting::item2_cost; stamina = BuildCSetting::item2_stamina; item_name = BuildCSetting::item2_name; }

            if (DC->hero->can_afford(cost)) {
                DC->hero->add_stamina(stamina);
                DC->hero->reduce_deposit(cost);

                // ✅ 重點：統一走 take_food（扣 1 份 + 手機顯示「你拿走」+ 剩幾份）
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
// ======================================================

// ==================== spawn resource ====================
void Build_C::child_update() {
    DataCenter* DC = DataCenter::get_instance();

    if (StateC == BuildStateC::Nothing) {
        if (DC->ui->is_open()) return;

        frames_passed += 1;
        if (frames_passed < interval_frames) return;
        frames_passed = 0;

        float r = std::rand() / (float)RAND_MAX;
        if (r < cur_prob) {
            cur_prob = base_prob;

            // ✅ 一次出 3 份（照你 Build_A/Build_B 的邏輯）
            set_food_amount(3);

            DC->phone->add_notification(
                BuildCSetting::phone_from,
                "進貨 / 上架了",
                "快來看看吧"
            );
        } else {
            cur_prob += prob_step;
            if (cur_prob > max_prob) cur_prob = max_prob;
        }
    }
}

void Build_C::child_init() {
    phone_key.clear();
    StateC = BuildStateC::Nothing;
    food_amount = 0;
    targeted_by_monster = false;
    in_confirm = false;
    pending_item = 0;
    ui_message.clear();
    ui_message_timer = 0;
}
// ======================================================

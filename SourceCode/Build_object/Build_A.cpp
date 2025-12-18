#include "Build_A.h"

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

// =========================
// Phone key helper
// =========================
static std::string make_buildA_phone_key(const Build_A* self) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "BUILD_A_%p", (void*)self);
    return std::string(buf);
}

// =========================
// BuildA settings
// =========================
namespace BuildASetting {
    double bento_stamina = 50;
    int    bento_cost    = 80;
    double drink_stamina = 20;
    int    drink_cost    = 50;

    static constexpr const char* img_shop  = "./assets/image/A_ui/restau.jpg";
    static constexpr const char* img_drink = "./assets/image/A_ui/drink.jpg";
    static constexpr const char* img_bento = "./assets/image/A_ui/bento.png";
}

// =========================
// Phone sync / food logic (第二份為主)
// =========================
bool Build_A::has_food() const {
    return food_amount > 0 || StateA == BuildStateA::Food;
}

void Build_A::sync_phone() {
    DataCenter* DC = DataCenter::get_instance();
    if (!DC || !DC->phone) return;

    if (phone_key.empty())
        phone_key = make_buildA_phone_key(this);

    if (!has_food()) {
        DC->phone->upsert_food_status(
            phone_key.c_str(),
            "小吃部",
            "已售罄",
            "目前沒有剩餘餐點"
        );
        return;
    }

    char msg[128];
    std::snprintf(msg, sizeof(msg), "目前剩下 %d 份餐點", food_amount);

    DC->phone->upsert_food_status(
        phone_key.c_str(),
        "小吃部",
        "販售中",
        msg
    );
}

void Build_A::notify_food_spawn() {
    DataCenter* DC = DataCenter::get_instance();
    if (!DC || !DC->phone) return;

    if (phone_key.empty())
        phone_key = make_buildA_phone_key(this);

    char msg[128];
    std::snprintf(msg, sizeof(msg), "午餐開始販售！目前共有 %d 份", food_amount);

    DC->phone->upsert_food_status(
        phone_key.c_str(),
        "小吃部",
        "午餐已開始販售",
        msg
    );
}

void Build_A::notify_food_taken(const char* who) {
    DataCenter* DC = DataCenter::get_instance();
    if (!DC || !DC->phone) return;

    if (phone_key.empty())
        phone_key = make_buildA_phone_key(this);

    if (!has_food()) {
        DC->phone->upsert_food_status(
            phone_key.c_str(),
            "小吃部",
            "已售罄",
            "目前沒有剩餘餐點"
        );
        return;
    }

    char title[64];
    char msg[128];
    std::snprintf(title, sizeof(title), "%s拿走了一份食物", who);
    std::snprintf(msg, sizeof(msg), "剩下 %d 份", food_amount);

    DC->phone->upsert_food_status(
        phone_key.c_str(),
        "小吃部",
        title,
        msg
    );
}

void Build_A::set_stateA(BuildStateA s) {
    StateA = s;
}

void Build_A::set_food_amount(int n) {
    food_amount = n;

    if (food_amount <= 0) {
        food_amount = 0;
        StateA = BuildStateA::Nothing;
        sync_phone();
        return;
    }

    StateA = BuildStateA::Food;
    notify_food_spawn();  // ✅ 生成時通知一次
}

bool Build_A::take_food(int amount, const char* who) {
    if (amount <= 0) return false;
    if (!has_food()) return false;

    if (food_amount <= 0) food_amount = 1; // fallback：若只靠 StateA

    food_amount -= amount;
    if (food_amount <= 0) {
        food_amount = 0;
        StateA = BuildStateA::Nothing;
        notify_food_taken(who); // 會走售罄分支
        return true;
    }

    StateA = BuildStateA::Food;
    notify_food_taken(who);
    return true;
}

// =========================
// Interact / UI
// =========================
void Build_A::on_interact() {
    DataCenter* DC = DataCenter::get_instance();
    if (!DC->ui->is_open()) {
        DC->ui->open(this);
    } else if (DC->ui->get_target() == this) {
        DC->ui->close();
    } else {
        DC->ui->open(this);
    }
}

void Build_A::draw_ui(UI* ui, float x, float y, float w, float h) {
    FontCenter* FC = FontCenter::get_instance();
    ALLEGRO_FONT* font = FC->NotoSansCJK[FontSize::SMALL];

    float padding = 20.0f;

    switch (StateA) {
        case BuildStateA::Food: {
            if (!in_confirm) {
                al_draw_text(font, al_map_rgb(255,255,0),
                    x + w/2.0f, y + padding, ALLEGRO_ALIGN_CENTER, "小吃部");

                // ✅ 顯示剩餘份數（第二份的邏輯）
                char leftbuf[64];
                std::snprintf(leftbuf, sizeof(leftbuf), "剩餘份數：%d", food_amount);
                al_draw_text(font, al_map_rgb(200,200,200),
                    x + padding, y + padding + 25, 0, leftbuf);

                al_draw_text(font, al_map_rgb(255,255,255),
                    x + padding, y + padding + 55, 0,
                    "-(1) 花費 $50 買飲料，恢復 20 飽食度");

                al_draw_text(font, al_map_rgb(255,255,255),
                    x + padding, y + padding + 85, 0,
                    "-(2) 花費 $80 買便當，恢復 50 飽食度");
            } else {
                const char* item_name = (pending_item == 1 ? "飲料" : "便當");

                al_draw_text(font, al_map_rgb(255,255,0),
                    x + w/2.0f, y + padding, ALLEGRO_ALIGN_CENTER, "Confirm Purchase");

                char buf[100];
                std::snprintf(buf, sizeof(buf), "確定購買 %s?", item_name);

                al_draw_text(font, al_map_rgb(255,255,255),
                    x + padding, y + padding + 60, 0, buf);

                al_draw_text(font, al_map_rgb(255,255,255),
                    x + padding, y + padding + 90, 0, "按下 E 確認，Q 取消");
            }

            // 顯示錢不夠提示
            if (ui_message_timer > 0 && !ui_message.empty()) {
                al_draw_text(font, al_map_rgb(255,80,80),
                    x + padding, y + padding + 120, 0, ui_message.c_str());
            }
            break;
        }

        case BuildStateA::Nothing:
        default: {
            al_draw_text(font, al_map_rgb(255,255,0),
                x + w/2.0f, y + padding, ALLEGRO_ALIGN_CENTER, "Nothing here now");
            break;
        }
    }

    // 圖片
    ImageCenter* IC = ImageCenter::get_instance();
    const char* img_path = BuildASetting::img_shop;

    if (StateA == BuildStateA::Food && in_confirm) {
        if (pending_item == 1) img_path = BuildASetting::img_drink;
        else if (pending_item == 2) img_path = BuildASetting::img_bento;
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
                0, 0, bw, bh,
                img_x1, img_y1,
                img_x2 - img_x1,
                img_y2 - img_y1,
                0);
        }
    }
}

void Build_A::update_ui(UI* ui) {
    DataCenter* DC = DataCenter::get_instance();

    if (StateA != BuildStateA::Food || !has_food()) {
        return;
    }

    // UI message 倒數
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
        return;
    }

    // confirm page
    if (esc_pressed) {
        in_confirm = false;
        pending_item = 0;
        return;
    }

    if (!keyE_pressed) return;

    int cost = 0;
    double stamina = 0;
    const char* item_name = "";

    if (pending_item == 1) {
        cost = BuildASetting::drink_cost;
        stamina = BuildASetting::drink_stamina;
        item_name = "飲料";
    } else if (pending_item == 2) {
        cost = BuildASetting::bento_cost;
        stamina = BuildASetting::bento_stamina;
        item_name = "便當";
    } else {
        in_confirm = false;
        pending_item = 0;
        return;
    }

    if (!DC->hero->can_afford(cost)) {
        char msg[100];
        std::snprintf(msg, sizeof(msg), "錢不夠！需要 $%d，目前 $%d", cost, (int)DC->hero->get_deposit());
        ui_message = msg;
        ui_message_timer = 120;
        return;
    }

    // ✅ 成功購買：扣錢、加飽食、加分（保留第一份 add_score）
    DC->hero->add_stamina(stamina);
    DC->hero->reduce_deposit(cost);
    DC->hero->add_score(10);

    // ✅ 用第二份邏輯：買一份就 take_food 一份（扣到 0 才售罄 + phone 更新）
    take_food(1, "You");

    in_confirm = false;
    pending_item = 0;

    // 如果買完剛好售罄：UI 關掉（跟你原本一致）
    if (!has_food()) {
        DC->ui->close();
    }
}

// =========================
// Spawn logic
// =========================
void Build_A::child_update() {
    DataCenter* DC = DataCenter::get_instance();

    if (StateA == BuildStateA::Nothing) {
        if (DC->ui->is_open()) return;

        frames_passed += 1;
        if (frames_passed < interval_frames) return;
        frames_passed = 0;

        float r = std::rand() / (float)RAND_MAX;
        if (r < cur_prob) {
            debug_log("Shiao Chi Bu start to sell some lunch\n");
            cur_prob = base_prob;

            // ✅ 第二份為主：一次出 3 份
            set_food_amount(3);

            // 你原本的「通知」可以留著（但 phone 也會 upsert）
            DC->phone->add_notification(
                "小吃部",
                "午餐已開始販售",
                "教院的人已經瘋狂從山上跑下來搶午餐了"
            );
        } else {
            cur_prob += prob_step;
            if (cur_prob > max_prob) cur_prob = max_prob;
        }
    }
}

void Build_A::child_init() {
    phone_key.clear();
    // 建議初始化，避免亂值
    food_amount = 0;
    StateA = BuildStateA::Nothing;
    in_confirm = false;
    pending_item = 0;
    ui_message.clear();
    ui_message_timer = 0;
}

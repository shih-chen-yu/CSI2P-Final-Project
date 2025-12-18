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

// =====================
// Settings（用你第一份的店與價格）
// =====================
namespace BuildBSetting{
    // (1) 三明治：$100，+50
    double sandwich_stamina = 50;
    int    sandwich_cost    = 100;

    // (2) 飯糰：$80，+40
    double onigiri_stamina  = 40;
    int    onigiri_cost     = 80;

    static constexpr const char* img_shop     = "./assets/image/A_ui/water.jpg";
    static constexpr const char* img_sandwich = "./assets/image/A_ui/sandwich.jpg";
    static constexpr const char* img_onigiri  = "./assets/image/A_ui/omigiri.jpg";

    static constexpr const char* phone_from = "水漾餐廳";
}

// 每棟 building 唯一 key
static std::string make_buildB_phone_key(const Build_B* self) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "BUILD_B_%p", (void*)self);
    return std::string(buf);
}

// =====================
// Phone helpers
// =====================
void Build_B::notify_food_spawn() {
    DataCenter* DC = DataCenter::get_instance();
    if (!DC || !DC->phone) return;

    if (phone_key.empty()) phone_key = make_buildB_phone_key(this);

    char msg[128];
    std::snprintf(msg, sizeof(msg), "開始販售！目前共有 %d 份", food_amount);

    DC->phone->upsert_food_status(
        phone_key.c_str(),
        BuildBSetting::phone_from,
        "餐點已上架",
        msg
    );
}

void Build_B::notify_food_taken(const char* who) {
    DataCenter* DC = DataCenter::get_instance();
    if (!DC || !DC->phone) return;

    if (phone_key.empty()) phone_key = make_buildB_phone_key(this);

    if (!has_food()) {
        DC->phone->upsert_food_status(
            phone_key.c_str(),
            BuildBSetting::phone_from,
            "已售罄",
            "目前沒有剩餘餐點"
        );
        return;
    }

    char title[64];
    char msg[128];
    std::snprintf(title, sizeof(title), "%s拿走了一份", (who ? who : "有人"));
    std::snprintf(msg, sizeof(msg), "剩下 %d 份", food_amount);

    DC->phone->upsert_food_status(
        phone_key.c_str(),
        BuildBSetting::phone_from,
        title,
        msg
    );
}

void Build_B::sync_phone() {
    DataCenter* DC = DataCenter::get_instance();
    if (!DC || !DC->phone) return;

    if (phone_key.empty()) phone_key = make_buildB_phone_key(this);

    if (!has_food()) {
        DC->phone->upsert_food_status(
            phone_key.c_str(),
            BuildBSetting::phone_from,
            "已售罄",
            "目前沒有剩餘餐點"
        );
        return;
    }

    char msg[128];
    std::snprintf(msg, sizeof(msg), "目前剩下 %d 份餐點", food_amount);

    DC->phone->upsert_food_status(
        phone_key.c_str(),
        BuildBSetting::phone_from,
        "販售中",
        msg
    );
}

// =====================
// Core state
// =====================
bool Build_B::has_food() const {
    return (food_amount > 0) || (StateB == BuildStateB::Food);
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
    StateB = BuildStateB::Food;
    notify_food_spawn();   // 生成時通知一次
}

bool Build_B::take_food(int amount, const char* who) {
    if (amount <= 0) return false;
    if (!has_food()) return false;

    if (food_amount <= 0) food_amount = 1; // fallback

    food_amount -= amount;
    if (food_amount <= 0) {
        food_amount = 0;
        StateB = BuildStateB::Nothing;
        notify_food_taken(who); // 會走售罄分支
        return true;
    }

    StateB = BuildStateB::Food;
    notify_food_taken(who);
    return true;
}

// =====================
// UI
// =====================
void Build_B::on_interact() {
    DataCenter* DC = DataCenter::get_instance();
    if(!DC->ui->is_open()) DC->ui->open(this);
    else if (DC->ui->get_target() == this) DC->ui->close();
    else DC->ui->open(this);
}

void Build_B::draw_ui(UI* ui, float x, float y, float w, float h) {
    FontCenter* FC = FontCenter::get_instance();
    ALLEGRO_FONT* font = FC->NotoSansCJK[FontSize::SMALL];
    float padding = 20.0f;

    switch(StateB){
        case BuildStateB::Food:{
            if(!in_confirm){
                al_draw_text(font, al_map_rgb(255,255,0),
                    x + w/2.0f, y + padding, ALLEGRO_ALIGN_CENTER, "水漾餐廳");

                char leftbuf[64];
                std::snprintf(leftbuf, sizeof(leftbuf), "剩餘份數：%d", food_amount);
                al_draw_text(font, al_map_rgb(200,200,200),
                    x + padding, y + padding + 25, 0, leftbuf);

                al_draw_text(font, al_map_rgb(255,255,255),
                    x + padding, y + padding + 55, 0,
                    "-(1) 花費 $100 買三明治，恢復 50 飽食度");

                al_draw_text(font, al_map_rgb(255,255,255),
                    x + padding, y + padding + 85, 0,
                    "-(2) 花費 $80 買飯糰，恢復 40 飽食度");
            } else {
                const char* item_name = (pending_item == 1 ? "三明治" : "飯糰");

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
        default:{
            al_draw_text(font, al_map_rgb(255,255,0),
                x + w/2.0f, y + padding, ALLEGRO_ALIGN_CENTER, "Nothing here now");
            break;
        }
    }

    ImageCenter* IC = ImageCenter::get_instance();
    const char* img_path = BuildBSetting::img_shop;

    if (StateB == BuildStateB::Food && in_confirm) {
        img_path = (pending_item == 1 ? BuildBSetting::img_sandwich : BuildBSetting::img_onigiri);
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
            al_draw_scaled_bitmap(ui_img, 0,0,bw,bh, img_x1, img_y1, img_x2-img_x1, img_y2-img_y1, 0);
        }
    }
}

void Build_B::update_ui(UI* ui) {
    DataCenter* DC = DataCenter::get_instance();
    if (StateB != BuildStateB::Food || !has_food()) return;

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

    if (esc_pressed) {
        in_confirm = false;
        pending_item = 0;
        return;
    }

    if (!keyE_pressed) return;

    int cost = 0;
    double stamina = 0;
    const char* item_name = "";

    if (pending_item == 1) { cost = BuildBSetting::sandwich_cost; stamina = BuildBSetting::sandwich_stamina; item_name = "三明治"; }
    else if (pending_item == 2) { cost = BuildBSetting::onigiri_cost; stamina = BuildBSetting::onigiri_stamina; item_name = "飯糰"; }

    if (!DC->hero->can_afford(cost)) {
        char msg[100];
        std::snprintf(msg, sizeof(msg), "錢不夠！需要 $%d，目前 $%d", cost, (int)DC->hero->get_deposit());
        ui_message = msg;
        ui_message_timer = 120;
        return;
    }

    DC->hero->add_stamina(stamina);
    DC->hero->reduce_deposit(cost);
    DC->hero->add_score(10);

    take_food(1, "你"); // ✅ 扣 1 份 + phone 更新

    in_confirm = false;
    pending_item = 0;

    if (!has_food()) DC->ui->close(); // 售罄才關
}

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

            set_food_amount(3);  // ✅ 一次出 3 份

            DC->phone->add_notification(
                BuildBSetting::phone_from,
                "午餐已開始販售",
                "歡迎前來購買～"
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
    in_confirm = false;
    pending_item = 0;
    ui_message.clear();
    ui_message_timer = 0;
}

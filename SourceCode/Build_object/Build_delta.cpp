#include "Build_delta.h"

#include "../data/DataCenter.h"
#include "../data/FontCenter.h"
#include "../data/ImageCenter.h"   // ✅ merge1: UI 圖片
#include "../object/ui.h"
#include "../info/TimeInfo.h"
#include "../object/hero.h"
#include "../object/Phone.h"
#include "../Utils.h"

#include <algorithm>
#include <allegro5/allegro_font.h>
#include <cstdlib>
#include <cstdio>

#include "../data/SoundCenter.h"
#include "../info/StarveInfo.h"

static int get_current_year() {
    DataCenter* DC = DataCenter::get_instance();
    if (DC && DC->time_info) {
        return DC->time_info->get_level(); // 1~4
    }
    return 1;
}

namespace BuildDeltaSetting {
    constexpr float office_prob = 0.1f;
    constexpr float camp_prob   = 0.1f;

    constexpr double office_stamina = 30.0;
    constexpr double camp_stamina   = 50.0;

    constexpr float  camp_poison_prob   = 0.1f;
    constexpr double camp_poison_damage = 40.0;

    // ✅ merge1: UI 圖片
    constexpr const char* img_none   = "./assets/image/delta_ui/delta.jpg";
    constexpr const char* img_office = "./assets/image/delta_ui/food.jpg";
    constexpr const char* img_camp   = "./assets/image/delta_ui/food.jpg";
}

static constexpr const char* KEY_OFFICE = "台達館系辦剩食";
static constexpr const char* KEY_CAMP   = "台達館營隊剩食";

void Build_delta::on_interact() {
    DataCenter* DC = DataCenter::get_instance();

    if (!DC->ui->is_open()) DC->ui->open(this);
    else if (DC->ui->get_target() == this) DC->ui->close();
    else DC->ui->open(this);
}

// ✅ merge2 的同步邏輯集中（避免重複寫）
void Build_delta::sync_phone_status(DataCenter* DC) {
    if (office_count <= 0) {
        DC->phone->clear_food_status(KEY_OFFICE);
    } else {
        char msg2[64];
        std::snprintf(msg2, sizeof(msg2), "目前剩餘 %d 份，先到先拿！", office_count);
        DC->phone->upsert_food_status(
            KEY_OFFICE,
            "系辦會議結束，有多的點心可以領取！",
            msg2
        );
    }

    if (camp_count <= 0) {
        DC->phone->clear_food_status(KEY_CAMP);
    } else {
        char msg2[64];
        if (camp_count == 1) std::snprintf(msg2, sizeof(msg2), "只剩最後 1 份，要搶要快！");
        else std::snprintf(msg2, sizeof(msg2), "目前剩餘 %d 份，先到先拿！", camp_count);

        DC->phone->upsert_food_status(
            KEY_CAMP,
            "營隊結束，有免費餐盒！",
            msg2
        );
    }
}

void Build_delta::draw_ui(UI* ui, float x, float y, float w, float h) {
    FontCenter* FC = FontCenter::get_instance();
    ALLEGRO_FONT* font = FC->NotoSansCJK[FontSize::SMALL];

    float padding = 20.0f;

    // ===== 沒任何剩食可領 =====
    if (office_count <= 0 && camp_count <= 0) {
        al_draw_text(font, al_map_rgb(255,255,0), x + w/2.0f, y + padding, ALLEGRO_ALIGN_CENTER, "台達館");
        al_draw_text(font, al_map_rgb(255,255,255), x + padding, y + padding + 40, 0, "目前台達館沒有剩食可以領取。");

        // ✅ merge1: 就算沒食物也顯示回饋訊息
        if (result_timer > 0 && !result_message.empty()) {
            al_draw_text(font, al_map_rgb(255,255,255), x + w/2.0f, y + h - 40, ALLEGRO_ALIGN_CENTER, result_message.c_str());
        }

        // ✅ merge1: NoFood 圖片
        ImageCenter* IC = ImageCenter::get_instance();
        ALLEGRO_BITMAP* ui_img = IC->get(BuildDeltaSetting::img_none);
        if (ui_img) {
            float img_padding = 10.0f;
            float reserve_bottom = 60.0f;

            float img_x1 = x + padding;
            float img_x2 = x + w - padding;
            float img_y1 = y + h * 0.55f + img_padding;
            float img_y2 = y + h - reserve_bottom - img_padding;

            if (img_y2 > img_y1) {
                int bw = al_get_bitmap_width(ui_img);
                int bh = al_get_bitmap_height(ui_img);
                al_draw_scaled_bitmap(ui_img, 0,0,bw,bh, img_x1,img_y1, (img_x2-img_x1),(img_y2-img_y1), 0);
            }
        }
        return;
    }

    // ===== 有至少一種剩食可領 =====
    if (!in_confirm) {
        al_draw_text(font, al_map_rgb(255,255,0), x + w/2.0f, y + padding, ALLEGRO_ALIGN_CENTER, "台達館");
        float yy = y + padding + 40;

        if (office_count > 0 && camp_count > 0) {
            al_draw_text(font, al_map_rgb(255,255,255), x + padding, yy, 0, "-(1) 系辦剩食（免費）恢復 30 飽食度"); yy += 30;
            al_draw_text(font, al_map_rgb(255,255,255), x + padding, yy, 0, "-(2) 營隊剩食（免費）恢復 50 飽食度"); yy += 30;
            al_draw_text(font, al_map_rgb(200,200,200), x + padding, yy + 10, 0, "請按 1 或 2 選擇，E 確認，Q 取消");
        } else if (office_count > 0) {
            al_draw_text(font, al_map_rgb(255,255,255), x + padding, yy, 0, "-(1) 系辦剩食（免費）恢復 30 飽食度"); yy += 30;
            al_draw_text(font, al_map_rgb(200,200,200), x + padding, yy + 10, 0, "請按 1 選擇，E 確認，Q 取消");
        } else { // camp_count > 0
            al_draw_text(font, al_map_rgb(255,255,255), x + padding, yy, 0, "-(1) 營隊剩食（免費）恢復 50 飽食度"); yy += 30;
            al_draw_text(font, al_map_rgb(200,200,200), x + padding, yy + 10, 0, "請按 1 選擇，E 確認，Q 取消");
        }
    } else {
        const char* item_name = "Unknown";
        double stamina = 0.0;

        if (pending_kind == DeltaFoodKind::Office) { item_name = "系辦剩食"; stamina = BuildDeltaSetting::office_stamina; }
        else if (pending_kind == DeltaFoodKind::Camp) { item_name = "營隊剩食"; stamina = BuildDeltaSetting::camp_stamina; }

        al_draw_text(font, al_map_rgb(255,255,0), x + w/2.0f, y + padding, ALLEGRO_ALIGN_CENTER, "Confirm Free Food");

        char buf[160];
        std::snprintf(buf, sizeof(buf), "確定要領取「%s」嗎？（免費，恢復 %.0f 飽食度）", item_name, stamina);
        al_draw_text(font, al_map_rgb(255,255,255), x + padding, y + padding + 60, 0, buf);
        al_draw_text(font, al_map_rgb(255,255,255), x + padding, y + padding + 90, 0, "按下 E 確認，Q 取消");
    }

    // ✅ merge1: 任何狀態都顯示回饋訊息
    if (result_timer > 0 && !result_message.empty()) {
        al_draw_text(font, al_map_rgb(255,255,255), x + w/2.0f, y + h - 40, ALLEGRO_ALIGN_CENTER, result_message.c_str());
    }

    // ✅ merge1: 確認頁顯示對應圖片（沒有確認就顯示 none）
    ImageCenter* IC = ImageCenter::get_instance();
    const char* img_path = BuildDeltaSetting::img_none;

    if (in_confirm) {
        if (pending_kind == DeltaFoodKind::Office) img_path = BuildDeltaSetting::img_office;
        else if (pending_kind == DeltaFoodKind::Camp) img_path = BuildDeltaSetting::img_camp;
        else img_path = BuildDeltaSetting::img_none;
    }

    ALLEGRO_BITMAP* ui_img = IC->get(img_path);
    if (ui_img) {
        float img_padding = 10.0f;
        float reserve_bottom = 60.0f;
        float img_x1 = x + padding;
        float img_x2 = x + w - padding;

        float img_y1 = y + h * 0.55f + img_padding;
        float img_y2 = y + h - reserve_bottom - img_padding;

        if (img_y2 > img_y1) {
            int bw = al_get_bitmap_width(ui_img);
            int bh = al_get_bitmap_height(ui_img);
            al_draw_scaled_bitmap(ui_img, 0,0,bw,bh, img_x1,img_y1, (img_x2-img_x1),(img_y2-img_y1), 0);
        }
    }
}

void Build_delta::update_ui(UI* ui) {
    DataCenter* DC = DataCenter::get_instance();

    if (result_timer > 0) result_timer--;

    if (office_count <= 0 && camp_count <= 0) return;

    bool key1_pressed = DC->key_state[ALLEGRO_KEY_1] && !DC->prev_key_state[ALLEGRO_KEY_1];
    bool key2_pressed = DC->key_state[ALLEGRO_KEY_2] && !DC->prev_key_state[ALLEGRO_KEY_2];
    bool keyQ_pressed = DC->key_state[ALLEGRO_KEY_Q] && !DC->prev_key_state[ALLEGRO_KEY_Q];
    bool keyE_pressed = DC->key_state[ALLEGRO_KEY_E] && !DC->prev_key_state[ALLEGRO_KEY_E];

    if (!in_confirm) {
        if (key1_pressed) {
            if (office_count > 0) { in_confirm = true; pending_kind = DeltaFoodKind::Office; }
            else if (camp_count > 0) { in_confirm = true; pending_kind = DeltaFoodKind::Camp; }
            return;
        }
        if (key2_pressed) {
            if (camp_count > 0) { in_confirm = true; pending_kind = DeltaFoodKind::Camp; }
            return;
        }
    } else {
        if (keyQ_pressed) {
            in_confirm = false;
            pending_kind = DeltaFoodKind::None;
            return;
        }

        if (keyE_pressed) {
            double stamina = 0.0;
            const char* msg_fmt = nullptr;

            if (pending_kind == DeltaFoodKind::Office) {
                stamina = BuildDeltaSetting::office_stamina;
                office_count--;
                DC->hero->add_score(10);

                if (auto* SC = SoundCenter::get_instance())
                    SC->play("./assets/sound/eat.wav", ALLEGRO_PLAYMODE_ONCE);

                msg_fmt = "你獲得了 %.0f 飽食度！";
            }
            else if (pending_kind == DeltaFoodKind::Camp) {
                float r = std::rand() / (float)RAND_MAX;
                if (r < BuildDeltaSetting::camp_poison_prob) {
                    stamina = -BuildDeltaSetting::camp_poison_damage;
                    msg_fmt = "你吃到過期的營隊剩食！飽食度 -%.0f！";
                    DC->hero->add_score(-100);

                    if (auto* SC = SoundCenter::get_instance())
                        SC->play("./assets/sound/vomit.mp3", ALLEGRO_PLAYMODE_ONCE);

                    DC->camera_shake_timer    = 0.4f;
                    DC->camera_shake_strength = 12.0f;
                    if (DC->starve_info) DC->starve_info->trigger_poison_flash();
                } else {
                    stamina = BuildDeltaSetting::camp_stamina;
                    DC->hero->add_score(30);
                    msg_fmt = "你獲得了 %.0f 飽食度！";
                    if (auto* SC = SoundCenter::get_instance())
                        SC->play("./assets/sound/eat.wav", ALLEGRO_PLAYMODE_ONCE);
                }
                camp_count--;
            }

            if (stamina != 0.0) DC->hero->add_stamina(stamina);

            if (msg_fmt) {
                char buf[96];
                double abs_val = (stamina >= 0 ? stamina : -stamina);
                std::snprintf(buf, sizeof(buf), msg_fmt, abs_val);
                result_message = buf;
                result_timer = 120;
            }

            // ✅ merge2: 吃完後同步手機剩餘
            sync_phone_status(DC);

            in_confirm = false;
            pending_kind = DeltaFoodKind::None;
        }
    }
}

void Build_delta::child_update() {
    DataCenter* DC = DataCenter::get_instance();

    frames_passed++;
    if (frames_passed < interval_frames) return;
    frames_passed = 0;

    int year = get_current_year();

    // ✅ 保持 office 狀態一直同步（merge2 原本就這樣）
    // office 這裡不做補貨（你 merge2 原本就是固定用 count 由其他邏輯決定）
    // 若你想 office 也像 camp 一樣補貨，我也可以照 camp 寫法補上。
    // 目前維持 merge2：只同步顯示
    // (office_count <=0 → clear; >0 → upsert)
    // camp 在沒貨時才補
    // ----------------------------------------------------

    // Camp：沒貨才補
    if (camp_count <= 0) {
        float r = std::rand() / (float)RAND_MAX;
        if (r < BuildDeltaSetting::camp_prob) {
            camp_count = std::max(1, 3 - (year / 2)); // merge2 難度曲線

            // ✅ 可選：merge1 的彩蛋通知（你想留就留）
            // DC->phone->add_notification("呂建德", "呂鑫 王晨志 吃午餐了嗎", "現在殺去台達");

        } else {
            // 沒補到貨就保持 0
            camp_count = 0;
        }
    }

    // ✅ 每次 tick 統一同步手機（避免狀態過期）
    sync_phone_status(DC);
}

void Build_delta::child_init() {
    frames_passed = 0;
    office_count  = 0;
    camp_count    = 0;

    in_confirm = false;
    pending_kind = DeltaFoodKind::None;

    result_message.clear();
    result_timer = 0;
}

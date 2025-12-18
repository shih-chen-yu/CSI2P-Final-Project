// Build_human.cpp
#include "Build_human.h"

#include "../data/DataCenter.h"
#include "../data/FontCenter.h"
#include "../data/ImageCenter.h"
#include "../object/ui.h"
#include "../object/hero.h"
#include "../object/Phone.h"
#include "../Utils.h"

#include <allegro5/allegro_font.h>
#include <cstdlib>   // rand

// ========= 可調整參數 =========
namespace BuildHumanSetting {
    // 講座相關
    constexpr float  lecture_prob     = 0.15f; // 每次檢查時講座出現機率
    constexpr double lecture_stamina  = 35.0;  // 講座便當恢復飽食度

    // 麥當勞相關
    constexpr int    mcd_cost         = 90;    // 價格
    constexpr double mcd_stamina      = 45.0;  // 麥當勞恢復飽食度
    constexpr int    mcd_cooldown_sec = 15;    // 冷卻秒數（期間不能再用人社院）

    // UI 顯示
    constexpr int    msg_frames       = 120;   // 結果訊息顯示 frame（約 2 秒，60FPS）

    static constexpr const char* img_human_main    = "./assets/image/human_ui/human.jpg";     // 人社院主頁
    static constexpr const char* img_lecture       = "./assets/image/human_ui/food.jpg";  // 領便當
    static constexpr const char* img_mcd           = "./assets/image/human_ui/mcdon.jpg";      // 麥當勞
    static constexpr const char* img_locked        = "./assets/image/human_ui/no_human.jpg";   // 無法進入(冷卻)
}

void Build_human::on_interact() {
    DataCenter* DC = DataCenter::get_instance();

    // 一般建築開關 UI 的邏輯
    if (!DC->ui->is_open()) {
        DC->ui->open(this);
    } else if (DC->ui->get_target() == this) {
        DC->ui->close();
    } else {
        DC->ui->open(this);
    }
}

void Build_human::draw_ui(UI* ui, float x, float y, float w, float h) {
    FontCenter* FC = FontCenter::get_instance();
    ALLEGRO_FONT* font = FC->NotoSansCJK[FontSize::SMALL];

    float padding = 20.0f;

    // 標題
    al_draw_text(
        font,
        al_map_rgb(255, 255, 0),
        x + w / 2.0f,
        y + padding,
        ALLEGRO_ALIGN_CENTER,
        "人文社會學院"
    );

    float yy = y + padding + 40.0f;

    // ===== 如果麥當勞冷卻中，直接顯示冷卻訊息，功能鎖住 =====
    if (mcd_cooldown_frames > 0) {
        int fps = (int)DataCenter::get_instance()->FPS;
        int remain_sec = (mcd_cooldown_frames + fps - 1) / fps;

        char buf[120];
        sprintf(buf, "你剛買完麥當勞，人社院的學生不歡迎你進來（約 %d 秒後可再使用）。", remain_sec);

        al_draw_text(
            font,
            al_map_rgb(255, 255, 255),
            x + padding,
            yy,
            0,
            buf
        );

        if (result_timer > 0 && !result_message.empty()) {
            al_draw_text(
                font,
                al_map_rgb(255, 255, 255),
                x + w / 2.0f,
                y + h - 40.0f,
                ALLEGRO_ALIGN_CENTER,
                result_message.c_str()
            );
        }
        

        // =========================
        // ⭐ 冷卻中：顯示「無法進入」圖片
        // =========================
        ImageCenter* IC = ImageCenter::get_instance();
        ALLEGRO_BITMAP* ui_img = IC->get(BuildHumanSetting::img_locked);

        if (ui_img) {
            float img_padding = 10.0f;
            float reserve_bottom = 60.0f; // 留給底部 result_message
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
                    img_x1, img_y1 + img_padding,
                    (img_x2 - img_x1),
                    (img_y2 - img_y1) - img_padding,
                    0
                );
            }
        }

        return;
    }

    // ===== 正常可使用狀態 =====
    if (!in_confirm) {
        int option_index_for_mcd = 1; // 先假設麥當勞會是 (1)，如果前面有講座就變成 (2)

        // 第一階段：選擇要做什麼
        if (lecture_available) {
            // 有講座便當 → 它是第 1 項
            al_draw_text(
                font,
                al_map_rgb(255, 255, 255),
                x + padding,
                yy,
                0,
                "- (1) 講座便當（免費）恢復 35 飽食度"
            );
            yy += 30.0f;

            option_index_for_mcd = 2; // 麥當勞變成第 2 項
        } else {
            // 沒講座便當 → 不顯示「沒有講座」，直接讓麥當勞是第 1 項
            option_index_for_mcd = 1;
        }

        // 麥當勞選項
        char line_mcd[160];
        sprintf(line_mcd,
                "- (%d) 購買麥當勞（$90，恢復 45 飽食度，購買後人社院將冷卻一段時間）",
                option_index_for_mcd);

        al_draw_text(
            font,
            al_map_rgb(255, 255, 255),
            x + padding,
            yy,
            0,
            line_mcd
        );
        yy += 40.0f;

        al_draw_text(
            font,
            al_map_rgb(200, 200, 200),
            x + padding,
            yy,
            0,
            "請按上方條目的數字鍵選擇，之後按 E 確認"
        );
    } else {
        // 第二階段：確認畫面
        const char* line1 = nullptr;
        const char* line2 = "按下 E 確認，Q 取消";

        switch (pending_choice) {
        case HumanChoice::Lecture:
            line1 = "確定要領取講座便當嗎？（免費，恢復 35 飽食度）";
            break;
        case HumanChoice::Mcdonald:
            line1 = "確定要購買麥當勞嗎？（$90，恢復 45 飽食度，會觸發冷卻）";
            break;
        default:
            line1 = "未選擇任何行動。";
            break;
        }

        al_draw_text(
            font,
            al_map_rgb(255, 255, 255),
            x + padding,
            yy,
            0,
            line1
        );
        yy += 30.0f;

        al_draw_text(
            font,
            al_map_rgb(200, 200, 200),
            x + padding,
            yy,
            0,
            line2
        );
    }

    // 底部結果訊息（吃完後顯示）
    if (result_timer > 0 && !result_message.empty()) {
        al_draw_text(
            font,
            al_map_rgb(255, 255, 255),
            x + w / 2.0f,
            y + h - 40.0f,
            ALLEGRO_ALIGN_CENTER,
            result_message.c_str()
        );
    }


    // =========================
    // ⭐ 正常狀態：依頁面決定圖片
    // =========================
    ImageCenter* IC = ImageCenter::get_instance();

    const char* img_path = BuildHumanSetting::img_human_main; // 預設：人社院主頁

    if (in_confirm) {
        // 確認頁：依 pending_choice 切圖
        if (pending_choice == HumanChoice::Lecture) {
            img_path = BuildHumanSetting::img_lecture;  // 領便當
        } else if (pending_choice == HumanChoice::Mcdonald) {
            img_path = BuildHumanSetting::img_mcd;      // 麥當勞
        }
    }

    ALLEGRO_BITMAP* ui_img = IC->get(img_path);
    if (ui_img) {
        float img_padding = 10.0f;
        float reserve_bottom = 60.0f; // 留給底部結果訊息
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
                img_x1, img_y1 + img_padding,
                (img_x2 - img_x1),
                (img_y2 - img_y1) - img_padding,
                0
            );
        }
    }
}

void Build_human::update_ui(UI* ui) {
    DataCenter* DC = DataCenter::get_instance();

    // 更新結果訊息計時器
    if (result_timer > 0) {
        result_timer--;
        if (result_timer <= 0)
            result_message.clear();
    }

    // 麥當勞冷卻中：不讓玩家操作其它功能
    if (mcd_cooldown_frames > 0) {
        return;
    }

    bool key1_pressed = DC->key_state[ALLEGRO_KEY_1] && !DC->prev_key_state[ALLEGRO_KEY_1];
    bool key2_pressed = DC->key_state[ALLEGRO_KEY_2] && !DC->prev_key_state[ALLEGRO_KEY_2];
    bool keyE_pressed = DC->key_state[ALLEGRO_KEY_E] && !DC->prev_key_state[ALLEGRO_KEY_E];
    bool esc_pressed  = DC->key_state[ALLEGRO_KEY_Q] && !DC->prev_key_state[ALLEGRO_KEY_Q];

    if (!in_confirm) {
        // ========== 第一階段：選擇項目 ==========
        // 規則：
        // 1. 如果「講座存在」→ 1 = 講座，2 = 麥當勞
        // 2. 如果「講座不存在」→ 1 = 麥當勞（唯一選項），2 沒作用

        if (key1_pressed) {
            if (lecture_available) {
                in_confirm     = true;
                pending_choice = HumanChoice::Lecture;
                debug_log("Human: choose lecture bento (via 1).\n");
            } else {
                in_confirm     = true;
                pending_choice = HumanChoice::Mcdonald;
                debug_log("Human: choose McDonald's (only choice, via 1).\n");
            }
            return;
        }

        if (key2_pressed && lecture_available) {
            // 只有在「講座存在」時，2 才代表麥當勞
            in_confirm     = true;
            pending_choice = HumanChoice::Mcdonald;
            debug_log("Human: choose McDonald's (via 2).\n");
            return;
        }
    } else {
        // ========== 第二階段：確認 / 取消 ==========
        if (esc_pressed) {
            in_confirm     = false;
            pending_choice = HumanChoice::None;
            debug_log("Human: cancel choice.\n");
            return;
        }

        if (keyE_pressed) {
            // 確認執行選擇
            switch (pending_choice) {
            case HumanChoice::Lecture: {
                DC->hero->add_stamina(BuildHumanSetting::lecture_stamina);
                DC->hero->add_score(30);
                lecture_available = false;

                char buf[80];
                sprintf(buf, "你在人社院領到講座便當，飽食度 +%.0f！",
                        BuildHumanSetting::lecture_stamina);
                result_message = buf;
                result_timer   = BuildHumanSetting::msg_frames;

                debug_log("Human: lecture bento taken.\n");
                break;
            }
            case HumanChoice::Mcdonald: {
                // ⭐ 先檢查錢夠不夠
                if (!DC->hero->can_afford(BuildHumanSetting::mcd_cost)) {
                    result_message = "錢不夠，買不起麥當勞！";
                    result_timer   = BuildHumanSetting::msg_frames;
                    debug_log("Human: not enough money for McDonald's.\n");
                    break; // 不扣錢、不加飽食、不進冷卻
                }

                DC->hero->reduce_deposit(BuildHumanSetting::mcd_cost);
                DC->hero->add_stamina(BuildHumanSetting::mcd_stamina);
                DC->hero->add_score(10);

                int fps = (int)DC->FPS;
                mcd_cooldown_frames = fps * BuildHumanSetting::mcd_cooldown_sec;

                char buf[100];
                sprintf(buf,
                        "你買了麥當勞，飽食度 +%.0f，人社院暫停使用約 %d 秒。",
                        BuildHumanSetting::mcd_stamina,
                        BuildHumanSetting::mcd_cooldown_sec);
                result_message = buf;
                result_timer   = BuildHumanSetting::msg_frames;

                debug_log("Human: McDonald's purchased.\n");
                break;
            }
            default:
                break;
            }

            in_confirm     = false;
            pending_choice = HumanChoice::None;
        }
    }
}

void Build_human::child_update() {
    DataCenter* DC = DataCenter::get_instance();

    // 麥當勞冷卻倒數
    if (mcd_cooldown_frames > 0) {
        mcd_cooldown_frames--;
    }

    // 講座事件：每隔 interval_frames 檢查一次
    frames_passed++;
    if (frames_passed < interval_frames) return;
    frames_passed = 0;

    if (!lecture_available) {
        float r = std::rand() / (float)RAND_MAX;
        if (r < BuildHumanSetting::lecture_prob) {
            lecture_available = true;
            debug_log("Human: lecture event TRIGGERED.\n");

            // 可選：手機通知
            DC->phone->add_notification(
                "人社院講座便當",
                "人社院有講座正在發放免費午餐！",
                "人社院正在舉辦文學講座，現場有免費便當可以領取～"
            );
        }
    }
}

void Build_human::child_init() {
    in_confirm         = false;
    pending_choice     = HumanChoice::None;
    result_message.clear();
    result_timer       = 0;

    lecture_available  = false;
    frames_passed      = 0;

    mcd_cooldown_frames = 0;
}

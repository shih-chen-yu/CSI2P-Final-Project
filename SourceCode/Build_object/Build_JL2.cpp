#include "Build_JL2.h"

#include "../data/DataCenter.h"
#include "../data/FontCenter.h"
#include "../data/ImageCenter.h"
#include "../object/ui.h"
#include "../object/hero.h"
#include "../object/Phone.h"
#include "../Utils.h"

#include <allegro5/allegro_font.h>
#include <allegro5/allegro5.h>
#include <cstdlib>

// ========= 可調整參數 =========
namespace BuildJL2Setting {
    // 每隔 interval_frames roll 一次
    constexpr float exam_prob       = 0.10f;  // 突發考試機率
    constexpr int   exam_duration_s = 20;     // 要在幾秒內回到炯朗館
    constexpr int   exam_penalty    = 80;     // 缺考扣多少錢

    constexpr float prof_prob       = 0.12f;  // 教授任務出現機率
    constexpr int   prof_reward     = 120;    // 完成任務獎勵多少錢
    constexpr int   prof_cd_s       = 30;     // 任務完成後冷卻秒數

    constexpr int   msg_frames      = 120;    // 底部提示顯示多久（frame）

    static constexpr const char* img_main = "./assets/image/edu.jpg";   // 主頁面
    static constexpr const char* img_prof = "./assets/image/edu.jpg";   // 教授任務(確認頁)
}

void Build_JL2::on_interact() {
    DataCenter* DC = DataCenter::get_instance();

    // 進館時先處理「突發考試」相關狀態
    if (exam_active) {
        // 代表玩家在時限內趕回炯朗館
        exam_active          = false;
        exam_penalty_pending = false;

        char buf[128];
        sprintf(buf, "你準時回到教育館參加突發考試，成功避免被扣錢！");
        result_message = buf;
        result_timer   = BuildJL2Setting::msg_frames;

        debug_log("JL: exam reached in time, no penalty.\n");
    } else if (exam_penalty_pending) {
        // 之前 miss 掉考試，這次進來才扣錢
        DC->hero->reduce_deposit(BuildJL2Setting::exam_penalty);
        exam_penalty_pending = false;

        char buf[128];
        sprintf(buf, "你上次沒來考試，被扣了 %d 元學雜費。", BuildJL2Setting::exam_penalty);
        result_message = buf;
        result_timer   = BuildJL2Setting::msg_frames;

        debug_log("JL: exam penalty applied.\n");
    }

    // 一般開關 UI 的邏輯
    if (!DC->ui->is_open()) {
        DC->ui->open(this);
    } else if (DC->ui->get_target() == this) {
        DC->ui->close();
    } else {
        DC->ui->open(this);
    }
}

void Build_JL2::draw_ui(UI* ui, float x, float y, float w, float h) {
    FontCenter* FC   = FontCenter::get_instance();
    ALLEGRO_FONT* f  = FC->NotoSansCJK[FontSize::SMALL];
    float padding    = 20.0f;
    float yy         = y + padding + 40.0f;

    // 標題
    al_draw_text(
        f,
        al_map_rgb(255, 255, 0),
        x + w / 2.0f,
        y + padding,
        ALLEGRO_ALIGN_CENTER,
        "教育館"
    );

    // 顯示目前是否有突發考試狀態（純提示）
    if (exam_active) {
        int fps = (int)DataCenter::get_instance()->FPS;
        int remain_sec = (exam_remain_frames + fps - 1) / fps;

        char buf[128];
        sprintf(buf, "【突發考試進行中】請在 %d 秒內回到教育館！（你已經在這裡）", remain_sec);
        al_draw_text(
            f,
            al_map_rgb(255, 180, 180),
            x + padding,
            yy,
            0,
            buf
        );
        yy += 30.0f;
    } else if (exam_penalty_pending) {
        al_draw_text(
            f,
            al_map_rgb(255, 120, 120),
            x + padding,
            yy,
            0,
            "你上次突發考試沒到，下次進教育館會被扣錢。"
        );
        yy += 30.0f;
    }

    // ===== 主體選單 =====
    if (!in_confirm) {
        int option_index = 1;

        // 教授任務（如果有）
        if (professor_task_available) {
            char buf[128];
            sprintf(
                buf,
                "- (%d) 接受教授臨時任務（完成後可獲得 $%d）",
                option_index,
                BuildJL2Setting::prof_reward
            );
            al_draw_text(
                f,
                al_map_rgb(255, 255, 255),
                x + padding,
                yy,
                0,
                buf
            );
            // 唯一選項 → 一定是 1
            option_index++;
            yy += 30.0f;
        } else {
            al_draw_text(
                f,
                al_map_rgb(200, 200, 200),
                x + padding,
                yy,
                0,
                "目前沒有教授臨時任務可以接。"
            );
            yy += 30.0f;
        }

        al_draw_text(
            f,
            al_map_rgb(200, 200, 200),
            x + padding,
            yy + 10.0f,
            0,
            "有可用選項時請按對應數字。"
        );
    } else {
        // ===== 確認畫面 =====
        const char* line1 = nullptr;

        switch (pending_choice) {
        case JL2Choice::ProfessorTask:
            line1 = "確定要接受教授臨時指派的任務嗎？完成後可獲得報酬。";
            break;
        default:
            line1 = "未選擇任何有效行動。";
            break;
        }

        al_draw_text(
            f,
            al_map_rgb(255, 255, 255),
            x + padding,
            yy,
            0,
            line1
        );
        yy += 30.0f;

        al_draw_text(
            f,
            al_map_rgb(200, 200, 200),
            x + padding,
            yy,
            0,
            "按下 E 確認，Q 取消。"
        );
    }

    // 底部結果訊息
    if (result_timer > 0 && !result_message.empty()) {
        al_draw_text(
            f,
            al_map_rgb(255, 255, 255),
            x + w / 2.0f,
            y + h - 40.0f,
            ALLEGRO_ALIGN_CENTER,
            result_message.c_str()
        );
    }

    ImageCenter* IC = ImageCenter::get_instance();

    const char* img_path = BuildJL2Setting::img_main; // 預設主頁

    if (in_confirm && pending_choice == JL2Choice::ProfessorTask) {
        img_path = BuildJL2Setting::img_prof;         // 教授任務確認頁
    }

    ALLEGRO_BITMAP* ui_img = IC->get(img_path);
    if (ui_img) {
        float img_padding = 10.0f;
        float reserve_bottom = 60.0f; // 留給底部 result_message
        float img_x1 = x + padding;
        float img_x2 = x + w - padding;

        // 圖片放在 UI 下半部
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

void Build_JL2::update_ui(UI* ui) {
    DataCenter* DC = DataCenter::get_instance();

    // 更新結果訊息計時
    if (result_timer > 0) {
        result_timer--;
        if (result_timer <= 0) {
            result_message.clear();
        }
    }

    // 冷卻中就單純顯示文字，不提供新的任務選擇
    if (professor_task_cooldown > 0) {
        professor_task_cooldown--;
    }

    bool key1_pressed = DC->key_state[ALLEGRO_KEY_1] && !DC->prev_key_state[ALLEGRO_KEY_1];
    bool keyE_pressed = DC->key_state[ALLEGRO_KEY_E] && !DC->prev_key_state[ALLEGRO_KEY_E];
    bool keyQ_pressed = DC->key_state[ALLEGRO_KEY_Q] && !DC->prev_key_state[ALLEGRO_KEY_Q];

    if (!in_confirm) {
        // ===== 第一階段：選擇行動 =====
        if (key1_pressed && professor_task_available) {
            in_confirm     = true;
            pending_choice = JL2Choice::ProfessorTask;
            debug_log("JL: choose professor task.\n");
            return;
        }
    } else {
        // ===== 第二階段：確認 / 取消 =====
        if (keyQ_pressed) {
            // 取消
            in_confirm     = false;
            pending_choice = JL2Choice::None;
            debug_log("JL: cancel choice.\n");
            return;
        }

        if (keyE_pressed) {
            switch (pending_choice) {
            case JL2Choice::ProfessorTask: {
                // 這裡簡化為「立刻幫教授忙完，立刻拿到錢」
                DC->hero->add_deposit(BuildJL2Setting::prof_reward);

                char buf[128];
                sprintf(buf, "你幫教授處理臨時任務，賺到 $%d！",
                        BuildJL2Setting::prof_reward);
                result_message          = buf;
                result_timer            = BuildJL2Setting::msg_frames;
                professor_task_available = false;

                // 啟動冷卻
                int fps = (int)DC->FPS;
                professor_task_cooldown = fps * BuildJL2Setting::prof_cd_s;

                debug_log("JL: professor task done, reward granted.\n");
                break;
            }
            default:
                break;
            }

            in_confirm     = false;
            pending_choice = JL2Choice::None;
        }
    }
}

void Build_JL2::child_update() {
    DataCenter* DC = DataCenter::get_instance();

    // ===== 突發考試倒數 =====
    if (exam_active) {
        if (exam_remain_frames > 0) {
            exam_remain_frames--;
            if (exam_remain_frames <= 0) {
                // 時限到了但玩家沒來
                exam_active          = false;
                exam_penalty_pending = true;

                debug_log("JL: exam failed, penalty will be applied next visit.\n");
                DC->phone->add_notification(
                    "教育館突發考試未出席",
                    "你沒有在時間內回到炯朗館考試，下次來系館記得交重補修費。",
                    "摳連。"
                );
            }
        }
    }

    // 教授任務冷卻在 update_ui 裡已經遞減

    // ===== 每隔 interval_frames roll 一次事件 =====
    frames_passed++;
    if (frames_passed < interval_frames) return;
    frames_passed = 0;

    // 1. 可能觸發新的突發考試（如果目前沒有 active / pending）
    if (!exam_active && !exam_penalty_pending) {
        float r = std::rand() / (float)RAND_MAX;
        if (r < BuildJL2Setting::exam_prob) {
            exam_active = true;

            int fps = (int)DC->FPS;
            exam_remain_frames = fps * BuildJL2Setting::exam_duration_s;

            debug_log("JL: exam event TRIGGERED.\n");
            DC->phone->add_notification(
                "教育館突發考試通知",
                "你被通知要到劉炯朗館參加突發小考，請在時限內回到炯朗館。",
                "沒考試的話會要重補修喔。"
            );
        }
    }

    // 2. 可能出現新的教授任務（沒有任務且不在冷卻時）
    if (!professor_task_available && professor_task_cooldown <= 0) {
        float r = std::rand() / (float)RAND_MAX;
        if (r < BuildJL2Setting::prof_prob) {
            professor_task_available = true;
            debug_log("JL: professor task spawned.\n");

            DC->phone->add_notification(
                "徵求工讀生",
                "教育館徵求一日工讀生協助活動進行",
                "意者請至清華學院院辦公室報名。"
            );
        }
    }
}

void Build_JL2::child_init() {
    in_confirm      = false;
    pending_choice  = JL2Choice::None;
    result_message.clear();
    result_timer    = 0;

    frames_passed   = 0;
    // interval_frames 已在成員變數裡給預設值

    exam_active          = false;
    exam_remain_frames   = 0;
    exam_penalty_pending = false;

    professor_task_available = false;
    professor_task_cooldown  = 0;
}
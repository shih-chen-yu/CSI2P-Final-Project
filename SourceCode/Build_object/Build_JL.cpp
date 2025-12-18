#include "Build_JL.h"

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
#include <cstdio>

// ========= 可調整參數 =========
namespace BuildJLSetting {
    constexpr float exam_prob       = 0.10f;  // 突發考試機率
    constexpr int   exam_duration_s = 20;     // 要在幾秒內回到炯朗館
    constexpr int   exam_penalty    = 80;     // 缺考扣多少錢

    constexpr float prof_prob       = 0.12f;  // 教授任務出現機率
    constexpr int   prof_reward     = 10;     // 完成任務獎勵多少錢
    constexpr int   prof_cd_s       = 30;     // 任務完成後冷卻秒數

    constexpr int   msg_frames      = 120;    // 底部提示顯示多久（frame）

    static constexpr const char* img_main = "./assets/image/JL_ui/JL.jpg";
    static constexpr const char* img_prof = "./assets/image/JL_ui/chhuang.jpg";
}

void Build_JL::on_interact() {
    DataCenter* DC = DataCenter::get_instance();

    // 規則：上次 miss 掉考試 → 下次進館才扣錢
    if (exam_penalty_pending) {
        DC->hero->reduce_deposit(BuildJLSetting::exam_penalty);
        exam_penalty_pending = false;

        // 手機狀態一起清掉（避免一直掛著）
        DC->phone->clear_food_status(Build_JL::PHONE_KEY_EXAM);

        char buf[128];
        std::snprintf(buf, sizeof(buf),
            "你上次沒來考試，被扣了 %d 元去重補修。", BuildJLSetting::exam_penalty);
        result_message = buf;
        result_timer   = BuildJLSetting::msg_frames;

        debug_log("JL: exam penalty applied.\n");
    }

    // UI 開關
    if (!DC->ui->is_open()) DC->ui->open(this);
    else if (DC->ui->get_target() == this) DC->ui->close();
    else DC->ui->open(this);
}

void Build_JL::draw_ui(UI* ui, float x, float y, float w, float h) {
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
        "劉炯朗館"
    );

    // 顯示目前是否有突發考試狀態（純提示）
    if (exam_active) {
        int fps = (int)DataCenter::get_instance()->FPS;
        int remain_sec = (exam_remain_frames + fps - 1) / fps;

        char buf[160];
        std::snprintf(buf, sizeof(buf),
            "【突發考試進行中】請在 %d 秒內回到炯朗館！（你已經在這裡）", remain_sec);
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
            "你上次突發考試沒到，下次記得去重補修。"
        );
        yy += 30.0f;
    }

    // ===== 主體選單 =====
    if (!in_confirm) {
        int option_index = 1;
        int exam_num = -1;
        int task_num = -1;

        if (exam_active) exam_num = option_index++;
        if (professor_task_available) task_num = option_index++;

        if (exam_num != -1) {
            int fps = (int)DataCenter::get_instance()->FPS;
            int remain_sec = (exam_remain_frames + fps - 1) / fps;

            char buf[160];
            std::snprintf(buf, sizeof(buf), "- (%d) 參加突發考試（剩 %d 秒）", exam_num, remain_sec);
            al_draw_text(f, al_map_rgb(255,255,255), x + padding, yy, 0, buf);
            yy += 30.0f;
        }

        if (task_num != -1) {
            char buf[160];
            std::snprintf(buf, sizeof(buf), "- (%d) 接受教授臨時任務（完成 +$%d）",
                          task_num, BuildJLSetting::prof_reward);
            al_draw_text(f, al_map_rgb(255,255,255), x + padding, yy, 0, buf);
            yy += 30.0f;
        }

        if (exam_num == -1 && task_num == -1) {
            al_draw_text(f, al_map_rgb(200,200,200), x + padding, yy, 0, "目前沒有可進行的事件。");
            yy += 30.0f;
        }

        al_draw_text(
            f,
            al_map_rgb(200, 200, 200),
            x + padding,
            yy + 10.0f,
            0,
            "按 1/2 選擇，E 確認，Q 取消。"
        );
    } else {
        const char* line1 = nullptr;
        switch (pending_choice) {
        case JLChoice::Exam:
            line1 = "確定要參加突發考試嗎？（按 E 確認）";
            break;
        case JLChoice::ProfessorTask:
            line1 = "確定要接受教授臨時指派的任務嗎？（按 E 確認）";
            break;
        default:
            line1 = "未選擇任何有效行動。";
            break;
        }

        al_draw_text(f, al_map_rgb(255,255,255), x + padding, yy, 0, line1);
        yy += 30.0f;
        al_draw_text(f, al_map_rgb(200,200,200), x + padding, yy, 0, "E 確認，Q 取消。");
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

    // 圖片區塊
    ImageCenter* IC = ImageCenter::get_instance();
    const char* img_path = BuildJLSetting::img_main;

    if (in_confirm && pending_choice == JLChoice::ProfessorTask) {
        img_path = BuildJLSetting::img_prof;
    }

    ALLEGRO_BITMAP* ui_img = IC->get(img_path);
    if (ui_img) {
        float img_padding = 10.0f;
        float reserve_bottom = 60.0f;
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

void Build_JL::update_ui(UI* ui) {
    DataCenter* DC = DataCenter::get_instance();

    if (result_timer > 0) {
        result_timer--;
        if (result_timer <= 0) result_message.clear();
    }

    bool key1_pressed = DC->key_state[ALLEGRO_KEY_1] && !DC->prev_key_state[ALLEGRO_KEY_1];
    bool key2_pressed = DC->key_state[ALLEGRO_KEY_2] && !DC->prev_key_state[ALLEGRO_KEY_2];
    bool keyE_pressed = DC->key_state[ALLEGRO_KEY_E] && !DC->prev_key_state[ALLEGRO_KEY_E];
    bool keyQ_pressed = DC->key_state[ALLEGRO_KEY_Q] && !DC->prev_key_state[ALLEGRO_KEY_Q];

    if (!in_confirm) {
        int option_index = 1;
        int exam_num = -1;
        int task_num = -1;

        if (exam_active) exam_num = option_index++;
        if (professor_task_available) task_num = option_index++;

        if (key1_pressed) {
            if (exam_num == 1) { in_confirm = true; pending_choice = JLChoice::Exam; return; }
            if (task_num == 1) { in_confirm = true; pending_choice = JLChoice::ProfessorTask; return; }
        }
        if (key2_pressed) {
            if (exam_num == 2) { in_confirm = true; pending_choice = JLChoice::Exam; return; }
            if (task_num == 2) { in_confirm = true; pending_choice = JLChoice::ProfessorTask; return; }
        }
        return;
    } else {
        if (keyQ_pressed) {
            in_confirm     = false;
            pending_choice = JLChoice::None;
            debug_log("JL: cancel choice.\n");
            return;
        }

        if (keyE_pressed) {
            switch (pending_choice) {
            case JLChoice::Exam: {
                exam_active = false;
                exam_penalty_pending = false;
                DC->phone->clear_food_status(Build_JL::PHONE_KEY_EXAM);
                result_message = "你準時回到炯朗館參加突發考試，成功避免被扣錢！";
                result_timer = BuildJLSetting::msg_frames;
                break;
            }
            case JLChoice::ProfessorTask: {
                DC->hero->add_deposit(BuildJLSetting::prof_reward);
                professor_task_available = false;
                DC->phone->clear_food_status(Build_JL::PHONE_KEY_TASK);

                int fps = (int)DC->FPS;
                professor_task_cooldown = fps * BuildJLSetting::prof_cd_s;

                result_message = "成功協助教授完成任務獲得報酬！";
                result_timer = BuildJLSetting::msg_frames;
                break;
            }
            default: break;
            }
            in_confirm = false;
            pending_choice = JLChoice::None;
        }
    }
}

void Build_JL::child_update() {
    DataCenter* DC = DataCenter::get_instance();

    if (professor_task_cooldown > 0) professor_task_cooldown--;

    // ===== 突發考試倒數 =====
    if (exam_active) {
        if (exam_remain_frames > 0) {
            exam_remain_frames--;
            if (exam_remain_frames <= 0) {
                exam_active          = false;
                exam_penalty_pending = true;

                debug_log("JL: exam failed, penalty will be applied next visit.\n");
                DC->phone->upsert_food_status(
                    Build_JL::PHONE_KEY_EXAM,
                    Build_JL::PHONE_APP_NAME,
                    "炯朗館突發考試未出席",
                    "你沒有在時間內回到炯朗館考試，下次進館會被扣錢。"
                );
            }
        }
    }

    // ===== roll event =====
    frames_passed++;
    if (frames_passed < interval_frames) return;
    frames_passed = 0;

    // 1) 觸發突發考試
    if (!exam_active && !exam_penalty_pending) {
        float r = std::rand() / (float)RAND_MAX;
        if (r < BuildJLSetting::exam_prob) {
            exam_active = true;

            int fps = (int)DC->FPS;
            exam_remain_frames = fps * BuildJLSetting::exam_duration_s;

            debug_log("JL: exam event TRIGGERED.\n");
            DC->phone->upsert_food_status(
                Build_JL::PHONE_KEY_EXAM,
                Build_JL::PHONE_APP_NAME,
                "突發考試通知",
                "請在時限內回到劉炯朗館。不來下次進館會被扣錢。"
            );
        }
    }

    // 2) 教授任務
    if (!professor_task_available && professor_task_cooldown <= 0) {
        float r = std::rand() / (float)RAND_MAX;
        if (r < BuildJLSetting::prof_prob) {
            professor_task_available = true;
            debug_log("JL: professor task spawned.\n");

            DC->phone->upsert_food_status(
                Build_JL::PHONE_KEY_TASK,
                Build_JL::PHONE_APP_NAME,
                "CHHuang教授徵求一位法律志工協助處理相關事務",
                "意者請至炯朗館洽詢CHHuang教授"
            );
        }
    }
}

void Build_JL::child_init() {
    in_confirm      = false;
    pending_choice  = JLChoice::None;
    result_message.clear();
    result_timer    = 0;

    frames_passed   = 0;

    exam_active          = false;
    exam_remain_frames   = 0;
    exam_penalty_pending = false;

    professor_task_available = false;
    professor_task_cooldown  = 0;
}

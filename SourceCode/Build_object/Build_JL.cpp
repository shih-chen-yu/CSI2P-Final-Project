#include "Build_JL.h"

#include "../data/DataCenter.h"
#include "../data/FontCenter.h"
#include "../object/ui.h"
#include "../object/hero.h"
#include "../object/Phone.h"
#include "../Utils.h"

#include <allegro5/allegro_font.h>
#include <allegro5/allegro5.h>
#include <cstdlib>

static constexpr const char* KEY_JL_EXAM = "JL_EXAM";
static constexpr const char* KEY_JL_TASK = "JL_TASK";

// ========= 可調整參數 =========
namespace BuildJLSetting {
    // 每隔 interval_frames roll 一次
    constexpr float exam_prob       = 0.10f;  // 突發考試機率
    constexpr int   exam_duration_s = 20;     // 要在幾秒內回到炯朗館
    constexpr int   exam_penalty    = 80;     // 缺考扣多少錢

    constexpr float prof_prob       = 0.12f;  // 教授任務出現機率
    constexpr int   prof_reward     = 120;    // 完成任務獎勵多少錢
    constexpr int   prof_cd_s       = 30;     // 任務完成後冷卻秒數

    constexpr int   msg_frames      = 120;    // 底部提示顯示多久（frame）
}

void Build_JL::on_interact() {
    DataCenter* DC = DataCenter::get_instance();

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

        char buf[128];
        sprintf(buf, "【突發考試進行中】請在 %d 秒內回到炯朗館！（你已經在這裡）", remain_sec);
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
            "你上次突發考試沒到，下次進炯朗館會被扣錢。"
        );
        yy += 30.0f;
    }

    if (!in_confirm) {
        int option_index = 1;
        int exam_num = -1;
        int task_num = -1;

        // 先決定編號（跟 update_ui 一樣）
        if (exam_active) exam_num = option_index++;
        if (professor_task_available) task_num = option_index++;

        // (Exam)
        if (exam_num != -1) {
            int fps = (int)DataCenter::get_instance()->FPS;
            int remain_sec = (exam_remain_frames + fps - 1) / fps;

            char buf[160];
            std::sprintf(buf, "- (%d) 參加突發考試（剩 %d 秒）", exam_num, remain_sec);
            al_draw_text(f, al_map_rgb(255,255,255), x + padding, yy, 0, buf);
            yy += 30.0f;
        }

        // (Professor task)
        if (task_num != -1) {
            char buf[160];
            std::sprintf(buf, "- (%d) 接受教授臨時任務（完成 +$%d）", task_num, BuildJLSetting::prof_reward);
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
            "按 1 / 2 選擇，E 確認，Q 取消。"
        );
    }
    else {
        // ===== 確認畫面 =====
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
}

void Build_JL::update_ui(UI* ui) {
    DataCenter* DC = DataCenter::get_instance();

    // 更新結果訊息計時
    if (result_timer > 0) {
        result_timer--;
        if (result_timer <= 0) {
            result_message.clear();
        }
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

        // 按 1
        if (key1_pressed) {
            if (exam_num == 1) { in_confirm = true; pending_choice = JLChoice::Exam; return; }
            if (task_num == 1) { in_confirm = true; pending_choice = JLChoice::ProfessorTask; return; }
        }

        // 按 2
        if (key2_pressed) {
            if (exam_num == 2) { in_confirm = true; pending_choice = JLChoice::Exam; return; }
            if (task_num == 2) { in_confirm = true; pending_choice = JLChoice::ProfessorTask; return; }
        }

        return;
    } else {
        // ===== 第二階段：確認 / 取消 =====
        if (keyQ_pressed) {
            // 取消
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
                DC->phone->clear_food_status(KEY_JL_EXAM);
                result_message = "你準時回到炯朗館參加突發考試，成功避免被扣錢！";
                result_timer = BuildJLSetting::msg_frames;
                break;
            }
            case JLChoice::ProfessorTask: {
                DC->hero->add_deposit(BuildJLSetting::prof_reward);
                professor_task_available = false;
                DC->phone->clear_food_status(KEY_JL_TASK);
                int fps = (int)DC->FPS;
                professor_task_cooldown = fps * BuildJLSetting::prof_cd_s;
                result_message = "你幫教授處理臨時任務，拿到報酬！";
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
    if (professor_task_cooldown > 0) professor_task_cooldown--;
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
                DC->phone->upsert_food_status(
                    KEY_JL_EXAM,
                    "炯朗館突發考試未出席",
                    "你沒有在時間內回到炯朗館考試，下次進館會被扣錢。"
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
        if (r < BuildJLSetting::exam_prob) {
            exam_active = true;

            int fps = (int)DC->FPS;
            exam_remain_frames = fps * BuildJLSetting::exam_duration_s;

            debug_log("JL: exam event TRIGGERED.\n");
            DC->phone->upsert_food_status(
                KEY_JL_EXAM,
                "劉炯朗館",
                "突發考試通知",
                "請在時限內回到劉炯朗館。不來下次進館會被扣錢。"
            );
        }
    }

    // 2. 可能出現新的教授任務（沒有任務且不在冷卻時）
    if (!professor_task_available && professor_task_cooldown <= 0) {
        float r = std::rand() / (float)RAND_MAX;
        if (r < BuildJLSetting::prof_prob) {
            professor_task_available = true;
            debug_log("JL: professor task spawned.\n");

            DC->phone->upsert_food_status(
                KEY_JL_TASK,
                "劉炯朗館",
                "劉炯朗館教授臨時任務",
                "有教授臨時需要人幫忙處理雜務，到劉炯朗館一趟也許可以賺點外快。"
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
    // interval_frames 已在成員變數裡給預設值

    exam_active          = false;
    exam_remain_frames   = 0;
    exam_penalty_pending = false;

    professor_task_available = false;
    professor_task_cooldown  = 0;
}
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
#include <cstdio>   // snprintf
#include <algorithm>

static constexpr const char* KEY_JL2_EXAM = "JL2_EXAM";
static constexpr const char* KEY_JL2_TASK = "JL2_TASK";

// ========= 可調整參數 =========
namespace BuildJL2Setting {
    constexpr float exam_prob       = 0.10f;  // 突發考試機率
    constexpr int   exam_duration_s = 20;     // 要在幾秒內回到教育館
    constexpr int   exam_penalty    = 80;     // 缺考扣多少錢

    constexpr float prof_prob       = 0.12f;  // 教授任務出現機率
    constexpr int   prof_reward     = 120;    // 完成任務獎勵多少錢
    constexpr int   prof_cd_s       = 30;     // 任務完成後冷卻秒數

    constexpr int   msg_frames      = 120;    // 底部提示顯示多久（frame）

    static constexpr const char* img_main = "./assets/image/edu.jpg";
    static constexpr const char* img_prof = "./assets/image/edu.jpg";
}

void Build_JL2::on_interact() {
    DataCenter* DC = DataCenter::get_instance();

    // ✅ 規則：上次 miss 掉考試 → 下次進館才扣錢
    if (exam_penalty_pending) {
        DC->hero->reduce_deposit(BuildJL2Setting::exam_penalty);
        exam_penalty_pending = false;

        // 手機狀態一起清掉（避免一直掛著）
        if (DC->phone) DC->phone->clear_food_status(KEY_JL2_EXAM);

        char buf[128];
        std::snprintf(buf, sizeof(buf),
            "你上次沒來考試，被扣了 %d 元學雜費。", BuildJL2Setting::exam_penalty);
        result_message = buf;
        result_timer   = BuildJL2Setting::msg_frames;

        debug_log("JL2: exam penalty applied.\n");
    }

    // UI 開關（跟 JL.cpp 同邏輯）
    if (!DC->ui->is_open()) DC->ui->open(this);
    else if (DC->ui->get_target() == this) DC->ui->close();
    else DC->ui->open(this);
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

    // 狀態提示
    if (exam_active) {
        int fps = (int)DataCenter::get_instance()->FPS;
        int remain_sec = (exam_remain_frames + fps - 1) / fps;

        char buf[160];
        std::snprintf(buf, sizeof(buf),
            "【突發考試進行中】請在 %d 秒內回到教育館！（你已經在這裡）", remain_sec);
        al_draw_text(f, al_map_rgb(255,180,180), x + padding, yy, 0, buf);
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

    // ===== 主體選單（跟 JL.cpp 一樣：可能有 exam / task 兩個選項）=====
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
            std::snprintf(buf, sizeof(buf),
                "- (%d) 參加突發考試（剩 %d 秒）", exam_num, remain_sec);
            al_draw_text(f, al_map_rgb(255,255,255), x + padding, yy, 0, buf);
            yy += 30.0f;
        }

        if (task_num != -1) {
            char buf[160];
            std::snprintf(buf, sizeof(buf),
                "- (%d) 接受臨時任務（完成 +$%d）", task_num, BuildJL2Setting::prof_reward);
            al_draw_text(f, al_map_rgb(255,255,255), x + padding, yy, 0, buf);
            yy += 30.0f;
        }

        if (exam_num == -1 && task_num == -1) {
            al_draw_text(f, al_map_rgb(200,200,200), x + padding, yy, 0,
                         "目前沒有可進行的事件。");
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
    } else {
        const char* line1 = nullptr;
        switch (pending_choice) {
        case JL2Choice::Exam:
            line1 = "確定要參加突發考試嗎？（按 E 確認）";
            break;
        case JL2Choice::ProfessorTask:
            line1 = "確定要接受臨時任務嗎？（按 E 確認）";
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
    const char* img_path = BuildJL2Setting::img_main;
    if (in_confirm && pending_choice == JL2Choice::ProfessorTask) img_path = BuildJL2Setting::img_prof;

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

void Build_JL2::update_ui(UI* ui) {
    DataCenter* DC = DataCenter::get_instance();

    // result 計時
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
            if (exam_num == 1) { in_confirm = true; pending_choice = JL2Choice::Exam; return; }
            if (task_num == 1) { in_confirm = true; pending_choice = JL2Choice::ProfessorTask; return; }
        }
        if (key2_pressed) {
            if (exam_num == 2) { in_confirm = true; pending_choice = JL2Choice::Exam; return; }
            if (task_num == 2) { in_confirm = true; pending_choice = JL2Choice::ProfessorTask; return; }
        }
        return;
    } else {
        if (keyQ_pressed) {
            in_confirm     = false;
            pending_choice = JL2Choice::None;
            debug_log("JL2: cancel choice.\n");
            return;
        }

        if (keyE_pressed) {
            switch (pending_choice) {
            case JL2Choice::Exam: {
                exam_active = false;
                exam_penalty_pending = false;
                if (DC->phone) DC->phone->clear_food_status(KEY_JL2_EXAM);

                result_message = "你準時回到教育館參加突發考試，成功避免被扣錢！";
                result_timer   = BuildJL2Setting::msg_frames;
                break;
            }
            case JL2Choice::ProfessorTask: {
                DC->hero->add_deposit(BuildJL2Setting::prof_reward);
                professor_task_available = false;
                if (DC->phone) DC->phone->clear_food_status(KEY_JL2_TASK);

                int fps = (int)DC->FPS;
                professor_task_cooldown = fps * BuildJL2Setting::prof_cd_s;

                result_message = "你完成臨時任務，拿到報酬！";
                result_timer   = BuildJL2Setting::msg_frames;
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

    // 教授任務冷卻
    if (professor_task_cooldown > 0) professor_task_cooldown--;

    // ===== 突發考試倒數 =====
    if (exam_active) {
        if (exam_remain_frames > 0) {
            exam_remain_frames--;
            if (exam_remain_frames <= 0) {
                exam_active          = false;
                exam_penalty_pending = true;

                debug_log("JL2: exam failed, penalty will be applied next visit.\n");
                if (DC->phone) {
                    DC->phone->upsert_food_status(
                        KEY_JL2_EXAM,
                        "教育館突發考試未出席",
                        "你沒有在時間內回到教育館，下次進館會被扣錢。"
                    );
                }
            }
        }
    }

    // ===== 每隔 interval_frames roll 一次事件 =====
    frames_passed++;
    if (frames_passed < interval_frames) return;
    frames_passed = 0;

    // 1) 可能觸發新的突發考試
    if (!exam_active && !exam_penalty_pending) {
        float r = std::rand() / (float)RAND_MAX;
        if (r < BuildJL2Setting::exam_prob) {
            exam_active = true;

            int fps = (int)DC->FPS;
            exam_remain_frames = fps * BuildJL2Setting::exam_duration_s;

            debug_log("JL2: exam event TRIGGERED.\n");
            if (DC->phone) {
                DC->phone->upsert_food_status(
                    KEY_JL2_EXAM,
                    "教育館",
                    "突發考試通知",
                    "請在時限內回到教育館。不來下次進館會被扣錢。"
                );
            }
        }
    }

    // 2) 可能出現新的臨時任務
    if (!professor_task_available && professor_task_cooldown <= 0) {
        float r = std::rand() / (float)RAND_MAX;
        if (r < BuildJL2Setting::prof_prob) {
            professor_task_available = true;
            debug_log("JL2: task spawned.\n");

            if (DC->phone) {
                DC->phone->upsert_food_status(
                    KEY_JL2_TASK,
                    "教育館",
                    "教育館臨時任務",
                    "有活動需要幫忙，到教育館也許可以賺點外快。"
                );
            }
        }
    }
}

void Build_JL2::child_init() {
    in_confirm      = false;
    pending_choice  = JL2Choice::None;
    result_message.clear();
    result_timer    = 0;

    frames_passed   = 0;

    exam_active          = false;
    exam_remain_frames   = 0;
    exam_penalty_pending = false;

    professor_task_available = false;
    professor_task_cooldown  = 0;
}

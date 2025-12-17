#include "Build_TSMC.h"

#include "../data/DataCenter.h"
#include "../data/FontCenter.h"
#include "../data/ImageCenter.h"
#include "../object/ui.h"
#include "../object/hero.h"
#include "../object/Phone.h"
#include "../Utils.h"

#include <allegro5/allegro_font.h>
#include <cstdlib>  // rand

// ====== 可調整參數 ======
namespace BuildTSMCSetting {
    // AMBA 剩食
    constexpr float  amba_prob      = 0.15f; // 每次檢查 AMBA 剩食機率
    constexpr double amba_stamina   = 40.0;  // 吃到便當恢復飽食度

    // 大老闆事件
    constexpr float  boss_prob      = 0.10f; // 每次檢查大老闆出現機率
    constexpr float  rob_success_p  = 0.6f;  // 打劫成功機率
    constexpr int    rob_reward     = 200;   // 成功時賺到的錢
    constexpr double beaten_starve  = 1.0;   // 失敗後飽食度直接變成 1

    // 7-11 食物
    constexpr float  store_prob     = 0.18f; // 每次檢查 7-11 有食物的機率
    constexpr int    store_cost     = 70;    // 價格
    constexpr double store_stamina  = 30.0;  // 回復飽食度

    // UI 顯示
    constexpr int    msg_frames     = 120;   // 結果訊息顯示 frame 數

    static constexpr const char* img_normal = "./assets/image/TSMC_ui/TSMC.jpg";
    static constexpr const char* img_rob    = "./assets/image/TSMC_ui/billgate.jpg";
    static constexpr const char* img_store  = "./assets/image/TSMC_ui/711.jpg";
    static constexpr const char* img_amba   = "./assets/image/TSMC_ui/food.jpg";
}

// ========== 基本互動開關 UI ==========
void Build_TSMC::on_interact() {
    DataCenter* DC = DataCenter::get_instance();

    if (!DC->ui->is_open()) {
        DC->ui->open(this);
    } else if (DC->ui->get_target() == this) {
        DC->ui->close();
    } else {
        DC->ui->open(this);
    }
}

// ========== UI 繪製 ==========

void Build_TSMC::draw_ui(UI* ui, float x, float y, float w, float h) {
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
        "台積館"
    );

    float yy = y + padding + 40.0f;

    // 如果目前完全沒有任何事件可用，就顯示一行敘述
    if (!amba_food_available && !boss_available && !store_food_available && !in_confirm) {
        al_draw_text(
            font,
            al_map_rgb(255, 255, 255),
            x + padding,
            yy,
            0,
            "現在台積館很平靜，沒有特別的事發生。"
        );
    } else if (!in_confirm) {
        // ===== 第一階段：顯示可選項目（從上到下編號 1,2,3...） =====
        int option_index = 1;

        if (amba_food_available) {
            char buf[128];
            sprintf(buf, "- (%d) AMBA 下課剩食便當（免費，恢復 %.0f 飽食度）",
                    option_index, BuildTSMCSetting::amba_stamina);
            al_draw_text(
                font,
                al_map_rgb(255, 255, 255),
                x + padding,
                yy,
                0,
                buf
            );
            yy += 30.0f;
            option_index++;
        }

        if (boss_available) {
            char buf[160];
            sprintf(buf,
                    "- (%d) 打劫路過的大老闆（成功 +%d 元；失敗飽食度變 1）",
                    option_index, BuildTSMCSetting::rob_reward);
            al_draw_text(
                font,
                al_map_rgb(255, 255, 255),
                x + padding,
                yy,
                0,
                buf
            );
            yy += 30.0f;
            option_index++;
        }

        if (store_food_available) {
            char buf[160];
            sprintf(buf,
                    "- (%d) 7-11 食物（$%d，恢復 %.0f 飽食度）",
                    option_index,
                    BuildTSMCSetting::store_cost,
                    BuildTSMCSetting::store_stamina);
            al_draw_text(
                font,
                al_map_rgb(255, 255, 255),
                x + padding,
                yy,
                0,
                buf
            );
            yy += 30.0f;
            option_index++;
        }

        if (option_index > 1) {
            al_draw_text(
                font,
                al_map_rgb(200, 200, 200),
                x + padding,
                yy + 10.0f,
                0,
                "請按 1 / 2 / 3 選擇條目，之後按 E 確認，ESC 離開"
            );
        }
    } else {
        // ===== 第二階段：確認畫面 =====
        const char* line1 = nullptr;

        switch (pending_choice) {
        case TSMCChoice::AMBAFood:
            line1 = "確定要去 AMBA 領剩食便當嗎？（免費補飽食度）";
            break;
        case TSMCChoice::RobBoss:
            line1 = "確定要打劫看起來很有錢的大老闆嗎？";
            break;
        case TSMCChoice::Buy711:
            line1 = "確定要在台積館 7-11 買食物嗎？";
            break;
        default:
            line1 = "沒有選擇任何行動。";
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
            "按下 E 確認，ESC 取消"
        );
    }

    // 底部結果訊息
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

    ImageCenter* IC = ImageCenter::get_instance();

    const char* img_path = BuildTSMCSetting::img_normal; // 預設：主頁正常

    if (in_confirm) {
        switch (pending_choice) {
        case TSMCChoice::RobBoss:
            img_path = BuildTSMCSetting::img_rob;
            break;
        case TSMCChoice::Buy711:
            img_path = BuildTSMCSetting::img_store;
            break;
        case TSMCChoice::AMBAFood:
            img_path = BuildTSMCSetting::img_amba;
            break;
        default:
            img_path = BuildTSMCSetting::img_normal;
            break;
        }
    }

    ALLEGRO_BITMAP* ui_img = IC->get(img_path);
    if (ui_img) {
        float img_padding = 10.0f;
        float reserve_bottom = 60.0f; // 留底部 result_message
        float img_x1 = x + padding;
        float img_x2 = x + w - padding;

        // 圖片放 UI 下半部
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

// ========== UI 邏輯 ==========

void Build_TSMC::update_ui(UI* ui) {
    DataCenter* DC = DataCenter::get_instance();

    // 更新結果訊息計時器
    if (result_timer > 0) {
        result_timer--;
        if (result_timer <= 0) {
            result_message.clear();
        }
    }

    bool key1_pressed = DC->key_state[ALLEGRO_KEY_1] && !DC->prev_key_state[ALLEGRO_KEY_1];
    bool key2_pressed = DC->key_state[ALLEGRO_KEY_2] && !DC->prev_key_state[ALLEGRO_KEY_2];
    bool key3_pressed = DC->key_state[ALLEGRO_KEY_3] && !DC->prev_key_state[ALLEGRO_KEY_3];
    bool keyE_pressed = DC->key_state[ALLEGRO_KEY_E] && !DC->prev_key_state[ALLEGRO_KEY_E];
    bool esc_pressed  = DC->key_state[ALLEGRO_KEY_ESCAPE] && !DC->prev_key_state[ALLEGRO_KEY_ESCAPE];

    if (!in_confirm) {
        // ===== 第一階段：選擇條目 =====
        if (!amba_food_available && !boss_available && !store_food_available) {
            // 沒東西可選，直接 return（只是進來看看）
            return;
        }

        // 跟 draw_ui 一樣的順序，從上到下給 1,2,3
        int option_index = 1;

        if (amba_food_available) {
            if ((option_index == 1 && key1_pressed) ||
                (option_index == 2 && key2_pressed) ||
                (option_index == 3 && key3_pressed)) {
                in_confirm     = true;
                pending_choice = TSMCChoice::AMBAFood;
                debug_log("TSMC: choose AMBA food.\n");
                return;
            }
            option_index++;
        }

        if (boss_available) {
            if ((option_index == 1 && key1_pressed) ||
                (option_index == 2 && key2_pressed) ||
                (option_index == 3 && key3_pressed)) {
                in_confirm     = true;
                pending_choice = TSMCChoice::RobBoss;
                debug_log("TSMC: choose Rob Boss.\n");
                return;
            }
            option_index++;
        }

        if (store_food_available) {
            if ((option_index == 1 && key1_pressed) ||
                (option_index == 2 && key2_pressed) ||
                (option_index == 3 && key3_pressed)) {
                in_confirm     = true;
                pending_choice = TSMCChoice::Buy711;
                debug_log("TSMC: choose 7-11 food.\n");
                return;
            }
            option_index++;
        }
    } else {
        // ===== 第二階段：確認 / 取消 =====
        if (esc_pressed) {
            in_confirm     = false;
            pending_choice = TSMCChoice::None;
            debug_log("TSMC: cancel choice.\n");
            return;
        }

        if (keyE_pressed) {
            auto* hero = DataCenter::get_instance()->hero;

            switch (pending_choice) {
            case TSMCChoice::AMBAFood: {
                hero->add_stamina(BuildTSMCSetting::amba_stamina);
                amba_food_available = false;

                char buf[100];
                sprintf(buf, "你去 AMBA 領到剩食便當，飽食度 +%.0f！",
                        BuildTSMCSetting::amba_stamina);
                result_message = buf;
                result_timer   = BuildTSMCSetting::msg_frames;

                debug_log("TSMC: AMBA food taken.\n");
                break;
            }
            case TSMCChoice::RobBoss: {
                float r = std::rand() / (float)RAND_MAX;
                if (r < BuildTSMCSetting::rob_success_p) {
                    // 打劫成功 → 加錢
                    hero->add_deposit(BuildTSMCSetting::rob_reward);

                    char buf[100];
                    sprintf(buf, "你成功打劫大老闆，獲得 %d 元！",
                            BuildTSMCSetting::rob_reward);
                    result_message = buf;
                } else {
                    // 失敗 → 飽食度變 1
                    double cur = hero->get_starve();
                    hero->add_stamina(BuildTSMCSetting::beaten_starve - cur);

                    result_message = "你被大老闆海扁一頓，飽食度只剩 1 點……";
                }
                boss_available = false;
                result_timer   = BuildTSMCSetting::msg_frames;

                debug_log("TSMC: Rob boss resolved.\n");
                break;
            }
            case TSMCChoice::Buy711: {
                // ⭐ 先檢查錢夠不夠
                if (!hero->can_afford(BuildTSMCSetting::store_cost)) {
                    result_message = "錢不夠，7-11 買不起……";
                    result_timer   = BuildTSMCSetting::msg_frames;

                    debug_log("TSMC: not enough money for 7-11.\n");
                    break; // 不扣錢、不加飽食、不消耗這次補貨
                }

                hero->reduce_deposit(BuildTSMCSetting::store_cost);
                hero->add_stamina(BuildTSMCSetting::store_stamina);
                store_food_available = false;

                char buf[120];
                sprintf(buf, "你在台積館 7-11 買了食物，花費 %d 元，飽食度 +%.0f。",
                        BuildTSMCSetting::store_cost,
                        BuildTSMCSetting::store_stamina);
                result_message = buf;
                result_timer   = BuildTSMCSetting::msg_frames;

                debug_log("TSMC: 7-11 food purchased.\n");
                break;
            }
            default:
                break;
            }

            in_confirm     = false;
            pending_choice = TSMCChoice::None;
        }
    }
}

// ========== 時間事件 ==========

void Build_TSMC::child_update() {
    DataCenter* DC = DataCenter::get_instance();

    // 每隔 interval_frames 檢查一次事件
    frames_passed++;
    if (frames_passed < interval_frames) return;
    frames_passed = 0;

    // 1. AMBA 剩食
    if (!amba_food_available) {
        float r = std::rand() / (float)RAND_MAX;
        if (r < BuildTSMCSetting::amba_prob) {
            amba_food_available = true;
            debug_log("TSMC: AMBA food event TRIGGERED.\n");

            DC->phone->add_notification(
                "台積館 有 剩食",
                "AMBA 課堂結束，有剩下的便當可以領！",
                "如標題。"
            );
        }
    }

    // 2. 大老闆來上課
    if (!boss_available) {
        float r = std::rand() / (float)RAND_MAX;
        if (r < BuildTSMCSetting::boss_prob) {
            boss_available = true;
            debug_log("TSMC: Boss event TRIGGERED.\n");

            DC->phone->add_notification(
                "台積館講座",
                "今日比爾蓋茲會來台積館演講。",
                "歡迎各位前往參加"
            );
        }
    }

    // 3. 7-11 補貨食物
    if (!store_food_available) {
        float r = std::rand() / (float)RAND_MAX;
        if (r < BuildTSMCSetting::store_prob) {
            store_food_available = true;
            debug_log("TSMC: 7-11 food event TRIGGERED.\n");

            DC->phone->add_notification(
                "台積館 7-11 補貨",
                "台積館 7-11 剛補了一批便當與點心。",
                "有點餓的話可以去看一下。"
            );
        }
    }
}

// ========== 初始化 ==========

void Build_TSMC::child_init() {
    in_confirm      = false;
    pending_choice  = TSMCChoice::None;
    result_message.clear();
    result_timer    = 0;

    amba_food_available  = false;
    boss_available       = false;
    store_food_available = false;

    frames_passed   = 0;
    interval_frames = 60 * 5;  // 你可以之後再調
}
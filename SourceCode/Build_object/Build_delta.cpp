#include "Build_delta.h"

#include "../data/DataCenter.h"
#include "../data/FontCenter.h"
#include "../data/ImageCenter.h"
#include "../object/ui.h"
#include "../object/hero.h"
#include "../object/Phone.h"
#include "../Utils.h"

#include <allegro5/allegro_font.h>
#include <allegro5/allegro_image.h>
#include <cstdlib>
#include "../data/SoundCenter.h"
#include "../info/StarveInfo.h"

namespace BuildDeltaSetting {
    constexpr float office_prob = 0.1f; // 系辦剩食機率
    constexpr float camp_prob   = 0.1f; // 營隊剩食機率

    // 回復 / 扣飽食度
    constexpr double office_stamina = 30.0; // 系辦剩食：恢復
    constexpr double camp_stamina   = 50.0; // 營隊剩食：正常恢復

    // ⭐ 營隊剩食：有機率是過期的
    constexpr float  camp_poison_prob   = 0.1f; // 壞掉的機率
    constexpr double camp_poison_damage = 40.0;  // 中毒時倒扣 40 飽食度

    constexpr const char* img_none   = "./assets/image/delta_ui/delta.jpg";
    constexpr const char* img_office = "./assets/image/delta_ui/food.jpg";
    constexpr const char* img_camp   = "./assets/image/delta_ui/food.jpg";
    constexpr const char* img_both   = "./assets/image/delta_ui/delta.jpg";
}

void Build_delta::on_interact() {
    DataCenter* DC = DataCenter::get_instance();

    if (!DC->ui->is_open()) {
        DC->ui->open(this);
    } else if (DC->ui->get_target() == this) {
        DC->ui->close();
    } else {
        DC->ui->open(this);
    }
}

void Build_delta::draw_ui(UI* ui, float x, float y, float w, float h) {
    FontCenter* FC = FontCenter::get_instance();
    ALLEGRO_FONT* font = FC->NotoSansCJK[FontSize::SMALL];

    float padding = 20.0f;

    enum class DeltaUIPage {
        NoFood,
        Select,
        Confirm
    };
    DeltaUIPage page;
    if (!office_available && !camp_available) page = DeltaUIPage::NoFood;
    else if (in_confirm)                      page = DeltaUIPage::Confirm;
    else                                      page = DeltaUIPage::Select;

    // ===== 沒任何剩食可領 =====
    if (!office_available && !camp_available) {
        al_draw_text(
            font,
            al_map_rgb(255, 255, 0),
            x + w / 2.0f,
            y + padding,
            ALLEGRO_ALIGN_CENTER,
            "Delta Hall"
        );
        al_draw_text(
            font,
            al_map_rgb(255, 255, 255),
            x + padding,
            y + padding + 40,
            0,
            "台達館沒有任何事情發生"
        );

        // ⭐ 就算沒有剩食，也要顯示剛剛吃完的回饋訊息
        if (result_timer > 0) {
            al_draw_text(
                font,
                al_map_rgb(255, 255, 255),
                x + w / 2.0f,
                y + h - 40,
                ALLEGRO_ALIGN_CENTER,
                result_message.c_str()
            );
        }
        // --- NoFood 頁面的圖片 ---
        {
            ImageCenter* IC = ImageCenter::get_instance();
            const char* img_path = BuildDeltaSetting::img_none; // 你自己新增一張
            ALLEGRO_BITMAP* ui_img = IC->get(img_path);

            if (ui_img) {
                float img_padding = 10.0f;
                float reserve_for_result = 60.0f;

                float img_x1 = x + padding;
                float img_x2 = x + w - padding;
                float img_y1 = y + h * 0.55f + img_padding;
                float img_y2 = y + h - reserve_for_result - img_padding;

                if (img_y2 > img_y1) {
                    int bw = al_get_bitmap_width(ui_img);
                    int bh = al_get_bitmap_height(ui_img);

                    al_draw_scaled_bitmap(
                        ui_img,
                        0, 0, bw, bh,
                        img_x1, img_y1, (img_x2 - img_x1), (img_y2 - img_y1),
                        0
                    );
                }
            }
        }
        return;
    }

    // ===== 有至少一種剩食可領 =====
    if (!in_confirm) {
        // 第一階段：顯示有哪些可以選
        al_draw_text(
            font,
            al_map_rgb(255, 255, 0),
            x + w / 2.0f,
            y + padding,
            ALLEGRO_ALIGN_CENTER,
            "台達館"
        );

        float yy = y + padding + 40;

        if (office_available && camp_available) {
            // 兩個選項都有：1 = 系辦，2 = 營隊
            al_draw_text(
                font,
                al_map_rgb(255, 255, 255),
                x + padding,
                yy,
                0,
                "-(1) 系辦剩食（免費）恢復 30 飽食度"
            );
            yy += 30;

            al_draw_text(
                font,
                al_map_rgb(255, 255, 255),
                x + padding,
                yy,
                0,
                "-(2) 營隊剩食（免費）恢復 50 飽食度"
            );
            yy += 30;

            al_draw_text(
                font,
                al_map_rgb(200, 200, 200),
                x + padding,
                yy + 10,
                0,
                "請按 對應數字鍵 選擇要領取的剩食"
            );
        }
        else if (office_available && !camp_available) {
            // 只有系辦剩食：只有 1 有效
            al_draw_text(
                font,
                al_map_rgb(255, 255, 255),
                x + padding,
                yy,
                0,
                "-(1) 系辦剩食（免費）恢復 30 飽食度"
            );
            yy += 30;

            al_draw_text(
                font,
                al_map_rgb(200, 200, 200),
                x + padding,
                yy + 10,
                0,
                "請按 對應數字鍵 選擇要領取的剩食"
            );
        }
        else if (!office_available && camp_available) {
            // 只有營隊剩食：只有 1 有效（UI 也只寫 1）
            al_draw_text(
                font,
                al_map_rgb(255, 255, 255),
                x + padding,
                yy,
                0,
                "-(1) 營隊剩食（免費）恢復 50 飽食度"
            );
            yy += 30;

            al_draw_text(
                font,
                al_map_rgb(200, 200, 200),
                x + padding,
                yy + 10,
                0,
                "請按 對應數字鍵 選擇要領取的剩食"
            );
        }
    } else {
        // 第二階段：確認畫面
        const char* item_name = nullptr;
        double stamina = 0.0;

        switch (pending_kind) {
            case DeltaFoodKind::Office:
                item_name = "系辦剩食";
                stamina   = BuildDeltaSetting::office_stamina;
                break;
            case DeltaFoodKind::Camp:
                item_name = "營隊剩食";
                stamina   = BuildDeltaSetting::camp_stamina;
                break;
            default:
                item_name = "Unknown";
                break;
        }

        al_draw_text(
            font,
            al_map_rgb(255, 255, 0),
            x + w / 2.0f,
            y + padding,
            ALLEGRO_ALIGN_CENTER,
            "Confirm Free Food"
        );

        char buf[120];
        sprintf(buf, "確定要領取「%s」嗎？（免費，恢復 %.0f 飽食度）", item_name, stamina);
        al_draw_text(
            font,
            al_map_rgb(255, 255, 255),
            x + padding,
            y + padding + 60,
            0,
            buf
        );

        al_draw_text(
            font,
            al_map_rgb(255, 255, 255),
            x + padding,
            y + padding + 90,
            0,
            "按下E確認，Q 取消"
        );
    }

    // ⭐ 其他狀態下也顯示回饋訊息（如果有）
    if (result_timer > 0) {
        al_draw_text(
            font,
            al_map_rgb(255, 255, 255),
            x + w / 2.0f,
            y + h - 40,
            ALLEGRO_ALIGN_CENTER,
            result_message.c_str()
        );
    }


    ImageCenter* IC = ImageCenter::get_instance();

    // 依「頁面」決定要顯示哪張圖（不是依有沒有食物）
    const char* img_path = BuildDeltaSetting::img_none;

    // Confirm 頁：可以再依 pending_kind 決定要顯示哪張（更合理）
    if (in_confirm) {
        if (pending_kind == DeltaFoodKind::Office)
            img_path = BuildDeltaSetting::img_office;
        else if (pending_kind == DeltaFoodKind::Camp)
            img_path = BuildDeltaSetting::img_camp;
        else
            img_path = BuildDeltaSetting::img_none;
    }

    ALLEGRO_BITMAP* ui_img = IC->get(img_path);
    if (ui_img) {
        // 預留底部顯示區：避開最下面的 result_message（你是 y+h-40）
        float img_padding = 10.0f;
        float img_h_reserve_for_result = 60.0f; // 給結果訊息留空間
        float img_top = y + h * 0.55f;          // 圖片從 UI 下半部開始（可調）
        float img_bottom = y + h - img_h_reserve_for_result;

        float img_x1 = x + padding;
        float img_x2 = x + w - padding;
        float img_y1 = img_top + img_padding;
        float img_y2 = img_bottom - img_padding;

        if (img_y2 > img_y1) {
            int bw = al_get_bitmap_width(ui_img);
            int bh = al_get_bitmap_height(ui_img);

            float dst_w = (img_x2 - img_x1);
            float dst_h = (img_y2 - img_y1);

            // 直接填滿顯示區（想維持比例我也可以再幫你改成 letterbox）
            al_draw_scaled_bitmap(
                ui_img,
                0, 0, bw, bh,
                img_x1, img_y1, dst_w, dst_h,
                0
            );
        }
    }
}

void Build_delta::update_ui(UI* ui) {
    DataCenter* DC = DataCenter::get_instance();

    if (result_timer > 0) result_timer--;

    if (!office_available && !camp_available) {
        // 沒東西可領，UI 只顯示說明
        return;
    }

    bool key1_pressed = DC->key_state[ALLEGRO_KEY_1] && !DC->prev_key_state[ALLEGRO_KEY_1];
    bool key2_pressed = DC->key_state[ALLEGRO_KEY_2] && !DC->prev_key_state[ALLEGRO_KEY_2];
    bool esc_pressed  = DC->key_state[ALLEGRO_KEY_Q] && !DC->prev_key_state[ALLEGRO_KEY_Q];
    bool keyE_pressed = DC->key_state[ALLEGRO_KEY_E] && !DC->prev_key_state[ALLEGRO_KEY_E];


    if (!in_confirm) {
        // 第一階段：選擇要領哪一種剩食

        // 情況 1：按 1 → 如果 office 有，選 office；如果 office 沒有但 camp 有 → 選 camp
        if (key1_pressed) {
            if (office_available) {
                in_confirm   = true;
                pending_kind = DeltaFoodKind::Office;
                debug_log("Delta: choose office leftovers (via 1).\n");
            } else if (camp_available) {
                in_confirm   = true;
                pending_kind = DeltaFoodKind::Camp;
                debug_log("Delta: choose camp leftovers (via 1, only choice).\n");
            }
            return;
        }

        // 情況 2：按 2 → 只有在 camp 有的時候才有效，office-only 時無效
        if (key2_pressed) {
            if (camp_available) {
                in_confirm   = true;
                pending_kind = DeltaFoodKind::Camp;
                debug_log("Delta: choose camp leftovers (via 2).\n");
            }
            return;
        }
    } else {
        // 第二階段：確認 / 取消
        if (esc_pressed) {
            in_confirm   = false;
            pending_kind = DeltaFoodKind::None;
            debug_log("Delta: cancel free food.\n");
            return;
        }

        // 一律按 E 確認
        if (keyE_pressed) {
            double stamina = 0.0;
            const char* msg_fmt = nullptr;

            if (pending_kind == DeltaFoodKind::Office) {
                // 系辦一定是正常的
                stamina = BuildDeltaSetting::office_stamina;
                office_available = false;
                msg_fmt = "你獲得了 %.0f 飽食度！";
            }
            else if (pending_kind == DeltaFoodKind::Camp) {
                // ⭐ 營隊有機率是過期的
                float r = std::rand() / (float)RAND_MAX;
                if (r < BuildDeltaSetting::camp_poison_prob) {
                    // 中毒：倒扣飽食度
                    stamina = -BuildDeltaSetting::camp_poison_damage;
                    msg_fmt = "你吃到過期的營隊剩食！飽食度 -%.0f！";
                    debug_log("Delta: camp leftovers POISONED.\n");
                     SoundCenter* SC = SoundCenter::get_instance();
                    if (SC) {
                        SC->play("./assets/sound/vomit.mp3", ALLEGRO_PLAYMODE_ONCE);
                    }
                    DC->camera_shake_timer    = 0.4f;  // 抖 0.4 秒
                    DC->camera_shake_strength = 12.0f; // 最大位移約 ±12 像素
                    if (DC->starve_info) {
                        DC->starve_info->trigger_poison_flash();
                    }

                } else {
                    // 正常：恢復飽食度
                    stamina = BuildDeltaSetting::camp_stamina;
                    msg_fmt = "你獲得了 %.0f 飽食度！";
                    debug_log("Delta: camp leftovers OK.\n");
                }
                camp_available = false;
            }

            // 實際套用到主角飽食度（支援正負）
            if (stamina != 0.0) {
                DC->hero->add_stamina(stamina);
            }

            // 顯示回饋訊息
            if (msg_fmt) {
                char buf[64];
                // 注意：顯示時數字用絕對值比較好看
                double abs_val = (stamina >= 0 ? stamina : -stamina);
                sprintf(buf, msg_fmt, abs_val);
                result_message = buf;
                result_timer   = 120; // 顯示約 2 秒
            }

            in_confirm   = false;
            pending_kind = DeltaFoodKind::None;

            return;
        }
    }
}

void Build_delta::child_update() {
    DataCenter* DC = DataCenter::get_instance();

    // 每隔 interval_frames 檢查一次
    frames_passed += 1;
    if (frames_passed < interval_frames) return;
    frames_passed = 0;

    // 系辦剩食事件：獨立機率、固定不調整
    if (!office_available) {
        float r = std::rand() / (float)RAND_MAX;
        if (r < BuildDeltaSetting::office_prob) {
            office_available = true;
            debug_log("Delta: office leftovers event TRIGGERED.\n");
            DC->phone->add_notification(
                "台達館系辦剩食",
                "系辦會議結束，有多的點心可以領取！",
                "快到台達館看看還有沒有你喜歡的。"
            );
        }
    }

    // 營隊剩食事件：獨立機率、固定不調整
    if (!camp_available) {
        float r = std::rand() / (float)RAND_MAX;
        if (r < BuildDeltaSetting::camp_prob) {
            camp_available = true;
            debug_log("Delta: camp leftovers event TRIGGERED.\n");
            DC->phone->add_notification(
                "台達館營隊剩食",
                "營隊剩下的早餐，一份20",
                "在台達217外面自取 錢錢自己放在旁邊～自由心證😍😍"
            );
        }
        if(r < BuildDeltaSetting::camp_prob / 2){
            debug_log("Delta: camp leftovers event TRIGGERED.\n");
            DC->phone->add_notification(
                "呂建德",
                "呂鑫 王晨志 吃午餐了嗎",
                "現在殺去台達"
            );
        }
    }
}

void Build_delta::child_init() {
    frames_passed = 0;
    office_available = false;
    camp_available   = false;
    in_confirm       = false;
    pending_kind     = DeltaFoodKind::None;
}
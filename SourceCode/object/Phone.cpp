#include "Phone.h"
#include "../data/DataCenter.h"
#include "../data/FontCenter.h"
#include "../Utils.h"

#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>


void Phone::init() {
    // 目前先不做任何初始化
    // 之後如果要給 Phone 一個圖示或位置，可以在這裡設定 shape 等
}

void Phone::update() {
    double now = al_get_time();

    // 用 iterator 走訪刪除（好懂）
    for (auto it = food_infos.begin(); it != food_infos.end(); ) {
        double age = now - it->create_time;
        if (age >= it->life_time) {
            it = food_infos.erase(it);  // erase 會回傳下一個 iterator
        } else {
            ++it;
        }
    }

}

void Phone::draw() {
    FontCenter *FC = FontCenter::get_instance();

    // 取得螢幕尺寸
    ALLEGRO_DISPLAY *disp = al_get_current_display();
    float dw = (float)al_get_display_width(disp);
    float dh = (float)al_get_display_height(disp);

    // ========== 背景遮罩（可選） ==========
    // 如果你希望開手機時畫面暗一點，就保留這段；
    // 如果只想畫右側一個面板，不要全畫面變暗，可以註解掉。
    al_draw_filled_rectangle(
        0, 0, dw, dh,
        al_map_rgba(0, 0, 0, 100)
    );

    // ========== 手機面板設定 ==========
    // 這裡我設計成右側豎立的一個長方形「手機」
    const float padding_screen = 20.0f;
    const float phone_w = 360.0f;
    const float phone_h = dh - padding_screen * 2.0f;
    const float phone_x = dw - phone_w - padding_screen;
    const float phone_y = padding_screen;

    // 手機背景（深灰＋微透明）
    al_draw_filled_rounded_rectangle(
        phone_x, phone_y,
        phone_x + phone_w, phone_y + phone_h,
        20.0f, 20.0f,
        al_map_rgba(32, 32, 32, 230)
    );

    // 手機邊框
    al_draw_rounded_rectangle(
        phone_x, phone_y,
        phone_x + phone_w, phone_y + phone_h,
        20.0f, 20.0f,
        al_map_rgb(255, 255, 255),
        2.0f
    );

    // ========== 標題 ==========
    ALLEGRO_FONT *title_font = FC->NotoSansCJK[FontSize::LARGE];
    ALLEGRO_FONT *text_font  = FC->NotoSansCJK[FontSize::SMALL];

    float padding_inner = 18.0f;
    float line_h = 24.0f;
    float cursor_y = phone_y + padding_inner;

    al_draw_text(
        title_font,
        al_map_rgb(255, 255, 255),
        phone_x + phone_w / 2.0f,
        cursor_y,
        ALLEGRO_ALIGN_CENTRE,
        "清交二手拍"
    );
    cursor_y += line_h * 2; // 留一點空白

    // ========== 沒有通知的情況 ==========
    if (food_infos.empty()) {
        al_draw_text(
            text_font,
            al_map_rgb(220, 220, 220),
            phone_x + padding_inner,
            cursor_y,
            ALLEGRO_ALIGN_LEFT,
            "目前沒有新的食物消息"
        );
        return;
    }

    // ========== 列出通知 ==========
    // 簡單版：每則通知顯示一行「[建築] message」
    // 之後你如果想顯示 content，可以再多畫一行或文字換行。
    for (const auto &info : food_infos) {
        // 避免超出手機下緣（需要兩行的空間）
        if (cursor_y + line_h * 2 > phone_y + phone_h - padding_inner) {
            al_draw_text(
                text_font,
                al_map_rgb(200, 200, 200),
                phone_x + padding_inner,
                cursor_y,
                ALLEGRO_ALIGN_LEFT,
                "... 還有更多通知"
            );
            break;
        }

        // 第一行：建築名稱 + 簡短訊息
        std::string line_title = "[" + info.building_name + "] " + info.message;

        al_draw_text(
            text_font,
            al_map_rgb(230, 230, 230),
            phone_x + padding_inner,
            cursor_y,
            ALLEGRO_ALIGN_LEFT,
            line_title.c_str()
        );
        cursor_y += line_h;

        // ★ 第二行：content（詳細內容）
        al_draw_text(
            text_font,
            al_map_rgb(200, 200, 200),
            phone_x + padding_inner + 12, // 縮排一點
            cursor_y,
            ALLEGRO_ALIGN_LEFT,
            info.content.c_str()
        );
        cursor_y += line_h + 4; // 再往下多留一點間距
    }
}

#include "ScoreInfo.h"

#include "../data/DataCenter.h"
#include "../data/FontCenter.h"

#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <cstdio>

ScoreInfo::ScoreInfo()
    : x(0), y(0), w(220), h(22), padding(12), score(0)
{}

void ScoreInfo::init() {
    DataCenter* DC = DataCenter::get_instance();

    // ✅ 畫面正中央上方
    x = DC->window_width / 2.0f - w / 2.0f;
    y = padding; // 上邊留一點距離
}

void ScoreInfo::update(int data) {
    score = data; // 只更新顯示
}

void ScoreInfo::draw() {
    DataCenter* DC = DataCenter::get_instance();
    FontCenter* FC = FontCenter::get_instance();
    if (!FC || !DC) return;

    // ===== 動態計算位置：螢幕正上方中央 =====
    float panel_w = 160.0f;
    float panel_h = 25.0f;
    float radius  = 10.0f;

    float cx = DC->window_width / 4.0f;
    float x1 = cx - panel_w / 2.0f;
    float y1 = 12.0f;
    float x2 = cx + panel_w / 2.0f;
    float y2 = y1 + panel_h;

    // ===== 外框（陰影感）=====
    al_draw_filled_rounded_rectangle(
        x1 - 2, y1 - 2,
        x2 + 2, y2 + 2,
        radius + 2, radius + 2,
        al_map_rgba(40, 45, 60, 220)
    );

    // ===== 主體底色（深藍灰，比純灰好看）=====
    al_draw_filled_rounded_rectangle(
        x1, y1,
        x2, y2,
        radius, radius,
        al_map_rgba(0, 0, 0, 100)
    );

    // ===== 分數文字 =====
    char buf[64];
    std::snprintf(buf, sizeof(buf), "SCORE  %d", score);

    ALLEGRO_FONT* font = FC->caviar_dreams[FontSize::SMALL];
    float text_y = y1 + (panel_h - al_get_font_line_height(font)) / 2.0f;

    al_draw_text(
        font,
        al_map_rgb(240, 240, 240),
        cx,
        text_y,
        ALLEGRO_ALIGN_CENTRE,
        buf
    );
}

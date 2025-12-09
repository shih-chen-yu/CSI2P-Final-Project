// TimeInfo.cpp
#include "TimeInfo.h"

#include "../data/DataCenter.h"
#include "../data/FontCenter.h"

#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>

TimeInfo::TimeInfo()
    : x(0), y(0), w(0), h(0),
      padding(0), level(1) {}

// 初始化位置、大小
void TimeInfo::init() {
    padding = 10.0f;

    // ⭐ 顯示在左上角
    w = 100.0f;
    h = 30.0f;
    x = padding;      // 左邊靠 padding
    y = padding;      // 上方靠 padding

    level = 1;
}

void TimeInfo::update(int data) {
    level = data;
}

void TimeInfo::draw() {
    FontCenter* FC = FontCenter::get_instance();

    // 背景條
    al_draw_filled_rounded_rectangle(
        x, y, x + w, y + h,
        5, 5,
        al_map_rgba(0, 0, 0, 160)  // 半透明黑
    );

    // 外框
    al_draw_rounded_rectangle(
        x, y, x + w, y + h,
        5, 5,
        al_map_rgb(255, 255, 255),
        2.0f
    );

    // 顯示文字: Lv. X
    char buf[32];
    sprintf(buf, "大學%d年級", level);

    al_draw_text(
        FC->NotoSansCJK[FontSize::SMALL],
        al_map_rgb(255, 255, 255),
        x + w / 2.0f,
        y + (h - al_get_font_line_height(FC->NotoSansCJK[FontSize::SMALL])) / 2.0f,
        ALLEGRO_ALIGN_CENTRE,
        buf
    );
}

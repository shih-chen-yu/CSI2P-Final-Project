#include "../data/DataCenter.h"
#include "../data/FontCenter.h"
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <algorithm>
#include "StarveInfo.h"
#include "../object/hero.h"

StarveInfo::StarveInfo(): x(0),y(0),w(150),h(18),padding(12),progress(0.0f) {}

void StarveInfo::init(){
    DataCenter* DC = DataCenter::get_instance();
    // 右上角，留一點 padding
    x = DC->window_width - padding - w;
    y = padding;
}

void StarveInfo::update(int data){
    // HERO::get_starve() 回傳 double，假設範圍 0..100
    double s = data;
    // 轉成 0..1（safe clamp）
    double t = s / 100.0;
    if(t < 0.0) t = 0.0;
    if(t > 1.0) t = 1.0;
    progress = static_cast<float>(t);
}

void StarveInfo::draw(){
    FontCenter* FC = FontCenter::get_instance();
    // 背景條
    al_draw_filled_rectangle(x - 2, y - 2, x + w + 2, y + h + 2, al_map_rgba(0,0,0,160));
    // 底色
    al_draw_filled_rectangle(x, y, x + w, y + h, al_map_rgba(80,80,80,200));
    // progress 條，顏色隨比例變化 (green->yellow->red)
    float r = progress < 0.5f ? (1.0f - progress*2.0f) : 0.0f;
    float g = progress < 0.5f ? (progress*2.0f) : (1.0f - (progress-0.5f)*2.0f);
    float b = progress > 0.5f ? (progress-0.5f)*2.0f : 0.0f;
    ALLEGRO_COLOR col = al_map_rgba_f(r, g, b, 1.0f);
    al_draw_filled_rectangle(x, y, x + w * progress, y + h, col);
    // 外框
    al_draw_rectangle(x, y, x + w, y + h, al_map_rgb(255,255,255), 2.0f);
    // 文字顯示百分比（需要 FontCenter 已初始化）
    if(FC){
        char buf[32];
        int pct = static_cast<int>(progress * 100.0f + 0.5f);
        sprintf(buf, "Starve: %d%%", pct);
        al_draw_text(FC->caviar_dreams[FontSize::SMALL], al_map_rgb(255,255,255),
                     x + w / 2.0f, y + h + 6.0f, ALLEGRO_ALIGN_CENTRE, buf);
    }
}
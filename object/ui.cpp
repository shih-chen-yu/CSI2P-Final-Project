#include "ui.h"
#include "../data/DataCenter.h"
#include "../object/Build.h"
#include <allegro5/allegro_font.h>

UI::UI(): open_flag(false), target_build(nullptr), x(0),y(0),w(0),h(0) {}

void UI::init(){
    DataCenter* DC = DataCenter::get_instance();
    w = DC->window_width * 0.5f;
    h = DC->window_height * 0.5f;
    x = (DC->window_width - w) / 2.0f;
    y = (DC->window_height - h) / 2.0f;
}

void UI::open(Build* target){
    target_build = target;
    open_flag = true;
}

void UI::close(){
    open_flag = false;
    target_build = nullptr;
}

bool UI::is_open() const { return open_flag; }

void UI::update(){
    if(!open_flag) return;
    DataCenter* DC = DataCenter::get_instance();
    // ESC 關閉 UI
    if(DC->key_state[ALLEGRO_KEY_ESCAPE]){
        close();
    }
    // 可以在這裡處理 UI 的按鈕/按鍵邏輯
}

void UI::draw(){
    if(!open_flag) return;
    // 半透明背景遮罩
    al_draw_filled_rectangle(0, 0, (float)al_get_display_width(al_get_current_display()), (float)al_get_display_height(al_get_current_display()), al_map_rgba(0,0,0,120));
    // 面板
    al_draw_filled_rectangle(x, y, x + w, y + h, al_map_rgba(32,32,32,220));
    al_draw_rectangle(x, y, x + w, y + h, al_map_rgb(255,255,255), 2.0f);
    // 可以在這裡顯示建築資訊、按鈕等等
}

Build* UI::get_target() const {
    return target_build;
}
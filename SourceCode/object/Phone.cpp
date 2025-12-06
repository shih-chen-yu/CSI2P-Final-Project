#include "Phone.h"
#include "../data/DataCenter.h"
#include "../Utils.h"

// 如果之後要畫字再加 FontCenter / allegro_font
// #include "../data/FontCenter.h"
// #include <allegro5/allegro_font.h>
// #include <allegro5/allegro_primitives.h>

void Phone::init() {
    // 目前先不做任何初始化
    // 之後如果要給 Phone 一個圖示或位置，可以在這裡設定 shape 等
}

void Phone::update() {
    // 目前 Game.cpp 只會在 phone->is_open() 為 true 時呼叫這裡
    // 你之後可以在這裡加按鍵邏輯，現在先留空讓 linker 過關
}

void Phone::draw() {
    if (!open) return;

    // 先放一個超簡單占位：之後你要畫 Phone UI 再改這裡
    // 例如之後可以畫一個手機框或選單
    // DataCenter* DC = DataCenter::get_instance();
    // 在這裡用 al_draw_rectangle / al_draw_text 畫東西
}

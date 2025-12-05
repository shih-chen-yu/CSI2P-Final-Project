#include "../data/DataCenter.h"
#include "../data/FontCenter.h"
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <algorithm>
#include "CoinInfo.h"
#include "../object/hero.h"

CoinInfo::CoinInfo(): x(0),y(0),w(150),h(18),coin(114514),padding(12) {}

void CoinInfo::init(){
    DataCenter* DC = DataCenter::get_instance();
    // 右上角，留一點 padding
    x = DC->window_width - ( padding + w ) * 2;
    y = padding;
}

void CoinInfo::update(int data){
    coin = data;
}

void CoinInfo::draw(){
    FontCenter* FC = FontCenter::get_instance();
    if(!FC) return;

    al_draw_filled_rectangle(
        x - 2, y - 2,
        x + w + 2, y + h + 2,
        al_map_rgba(0, 0, 0, 160)
    );
    al_draw_filled_rectangle(
        x, y,
        x + w, y + h,
        al_map_rgba(80, 80, 80, 200)
    );

    char buf[32];
    std::sprintf(buf, "Coin: %d", coin);

    ALLEGRO_FONT* font = FC->caviar_dreams[FontSize::SMALL];
    float text_y = y + (h - al_get_font_line_height(font)) / 2.0f;

    al_draw_text(
        font,
        al_map_rgb(255, 255, 0),
        x + w / 2.0f,
        text_y,
        ALLEGRO_ALIGN_CENTRE,
        buf
    );
}
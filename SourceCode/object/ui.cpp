#include "ui.h"
#include "../data/DataCenter.h"
#include "../object/Build.h"
#include "../data/ImageCenter.h"
#include "../data/FontCenter.h"
#include "../Utils.h"
#include "../Player.h"

#include <allegro5/allegro_font.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_ttf.h>

// ---- 這兩個是原本 UI.cpp 的設定，用來畫愛心與 coin ----
constexpr char love_img_path[] = "./assets/image/love.png";
constexpr int love_img_padding = 5;

// 檔案區域的靜態變數，不用改 ui.h
static ALLEGRO_BITMAP *love = nullptr;

UI::UI()
    : open_flag(false), target_build(nullptr),
      x(0), y(0), w(0), h(0) {}

// 初始化：計算面板位置 + 載入愛心圖片
void UI::init(){
    DataCenter* DC = DataCenter::get_instance();
    ImageCenter *IC = ImageCenter::get_instance();

    w = DC->window_width * 0.75f;
    h = DC->window_height * 0.75f;
    x = (DC->window_width - w) / 2.0f;
    y = (DC->window_height - h) / 2.0f;

    // 載入 HP 愛心圖
    love = IC->get(love_img_path);
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

Build* UI::get_target() const {
    return target_build;
}

void UI::update(){
    if(!open_flag) return;

    DataCenter* DC = DataCenter::get_instance();

    // ESC 關閉 UI
    if(DC->key_state[ALLEGRO_KEY_ESCAPE] &&
       !DC->prev_key_state[ALLEGRO_KEY_ESCAPE]) {
        close();
        return;
    }

    // 把按鍵邏輯交給對應的建築處理
    if(target_build){
        target_build->update_ui(this);
    }
}

void UI::draw(){
    DataCenter *DC = DataCenter::get_instance();
    FontCenter *FC = FontCenter::get_instance();

    // ========= 先畫「常駐 UI」：HP 愛心 + coin =========

    const int &game_field_length = DC->game_field_length;
    const int &player_HP = DC->player->HP;

    if (love) {
        int love_width = al_get_bitmap_width(love);
        for(int i = 1; i <= player_HP; ++i) {
            al_draw_bitmap(
                love,
                game_field_length - (love_width + love_img_padding) * i,
                love_img_padding,
                0
            );
        }
    }

    const int &player_coin = DC->player->coin;
    al_draw_textf(
        FC->courier_new[FontSize::MEDIUM], al_map_rgb(0, 0, 0),
        game_field_length + love_img_padding, love_img_padding,
        ALLEGRO_ALIGN_LEFT, "coin: %5d", player_coin
    );

    // ========= 再畫「建築的互動視窗」（如果有打開） =========

    if(!open_flag) return;

    // 半透明背景遮罩
    ALLEGRO_DISPLAY *disp = al_get_current_display();
    float dw = (float)al_get_display_width(disp);
    float dh = (float)al_get_display_height(disp);

    al_draw_filled_rectangle(
        0, 0, dw, dh,
        al_map_rgba(0, 0, 0, 120)
    );

    // 中央面板
    al_draw_filled_rectangle(
        x, y, x + w, y + h,
        al_map_rgba(32, 32, 32, 220)
    );
    al_draw_rectangle(
        x, y, x + w, y + h,
        al_map_rgb(255, 255, 255),
        2.0f
    );

    // 把面板內的內容交給建築畫（例如 Build_A 的菜單）
    if(target_build){
        target_build->draw_ui(this, x, y, w, h);
    }
}

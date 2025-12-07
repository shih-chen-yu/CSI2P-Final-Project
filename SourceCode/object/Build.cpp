#include "Build.h"
#include "ui.h"

#include "../Utils.h"

#include "../data/ImageCenter.h"
#include "../data/DataCenter.h"
#include "../shapes/Rectangle.h"

#include <allegro5/bitmap_draw.h>
#include <allegro5/allegro_primitives.h>
#include <cstring>

namespace Buildsetting{
    static constexpr char picfolder[50] = "./assets/image/building/";
}
void Build::init(){
    char buffer[100];
    
    // ★ 修正點：直接取得回傳值，不要加 .c_str()
    const char* baseName = sprite_basename(); 

    // ★ 防呆：如果讀到空字串（通常是因為 Level.txt 有空行），給個預設值以免當機
    if(baseName == nullptr || strlen(baseName) == 0) {
        printf("[Error] Empty basename detected in Build::init! Check Level.txt for empty lines.\n");
        baseName = "building"; // 暫時給個預設檔名防止崩潰
    }

    sprintf(buffer,
        "%s%s.png",
        Buildsetting::picfolder,
        baseName
    );
    main_picpath = std::string{buffer};

    // ★ 根據你的截圖，提示圖片應該是在 building 資料夾下
    // 原本是 "./assets/image/F.png" -> 改成下面這樣：
    hint_picpath = "./assets/image/F.png";

    ImageCenter* IC = ImageCenter::get_instance();
    ALLEGRO_BITMAP* pic = IC->get(main_picpath);
    
    // 如果圖片讀取失敗，這裡會再崩潰一次，所以最好檢查一下 pic 是否為 NULL
    if(!pic) {
        printf("[Error] Failed to load image: %s\n", main_picpath.c_str());
        return; // 或者做其他錯誤處理
    }

    DataCenter* DC = DataCenter::get_instance();
    scale = 75.0 / al_get_bitmap_width(pic);
    hint_scale = 0.2;

    shape.reset(new Rectangle{
        DC->window_width / 2 - al_get_bitmap_width(pic) * scale / 2,
        DC->window_height / 2 - al_get_bitmap_height(pic) * scale / 2,
        DC->window_width / 2 + al_get_bitmap_width(pic) * scale / 2,
        DC->window_height / 2 + al_get_bitmap_height(pic) * scale / 2
    });

    child_init();
}

void Build::draw(){
    // 取得當前狀態對應的建築物圖片
    ImageCenter* IC = ImageCenter::get_instance();
    ALLEGRO_BITMAP* pic = IC->get(main_picpath);
    
    float src_w = al_get_bitmap_width(pic); // 原始圖片大小
    float src_h = al_get_bitmap_height(pic);
    
    float dst_w = src_w * scale; // 縮放後要顯示的大小
    float dst_h = src_h * scale;
    
    float x = shape->center_x() - dst_w / 2 ; // 以 Rectangle 的中心為基準，計算出圖片左上角要畫在哪
    float y = shape->center_y() - dst_h / 2 ;

    al_draw_filled_rectangle( // 先畫 hitbox：半透明方塊
        x,
        y,
        x + dst_w,
        y + dst_h,
        al_map_rgba(100, 100, 100, 80)   // 綠色、80 透明度（數字越小越透明）
    );
    al_draw_scaled_bitmap( // 再畫縮放後的建築物圖片，位置與大小和 hitbox 對齊
        pic,
        0, 0, src_w, src_h,   // 原圖來源區域
        x, y, dst_w, dst_h,   // 縮放後的目標區域
        0
    );

    // ★ 如果目前在「可互動」狀態，畫提示圖
    if(State == BuildState::F_NOTIFY){
        ALLEGRO_BITMAP* hint = IC->get(hint_picpath);
        float hw = al_get_bitmap_width(hint);
        float hh = al_get_bitmap_height(hint);

        // 縮小一點：例如 0.7 倍
        float dst_hw = hw * hint_scale;
        float dst_hh = hh * hint_scale;

        // 畫在建築上方中央
        float hx = shape->center_x() - dst_hw / 2 + 0.5 * dst_w;
        float hy = shape->center_y() - dst_hh - 0.5 * dst_h;   // 上方留 10px

        al_draw_scaled_bitmap(
            hint,
            0, 0, hw, hh,
            hx, hy, dst_hw, dst_hh,
            0
        );
    }
}

void Build::update(){
    // 取得全域的輸入與 UI 狀態
    DataCenter* DC = DataCenter::get_instance();

    // 當使用者按住 F 鍵時
    if(DC->key_state[ALLEGRO_KEY_F] && !DC->prev_key_state[ALLEGRO_KEY_F]){
        // 只有在 F_NOTIFY 狀態下才會真正觸發「F 被按下」行為
        if(State == BuildState::F_NOTIFY){
            debug_log("Build F key pressed.\n");
            on_interact();
        }
    }

    child_update();
}

void Build::change_state(int changer){
    if(changer == 0){ // changer == 0：回到 NORMAL 狀態，並關閉 UI
        State = BuildState::NORMAL;
    }else if(changer == 1){ // changer == 1：切換成「F 提示」狀態（如果當前不是正在按 F）
        State = BuildState::F_NOTIFY;
    }
}

void Build::set_center(float cx, float cy){
    // 使用目前的 scale 與貼圖大小，設定 shape 的邊界以 cx/cy 為中心
    ImageCenter* IC = ImageCenter::get_instance();
    ALLEGRO_BITMAP* pic = IC->get(main_picpath);
    float src_w = al_get_bitmap_width(pic);
    float src_h = al_get_bitmap_height(pic);
    float half_w = src_w * scale * 0.5f;
    float half_h = src_h * scale * 0.5f;
    shape.reset(new Rectangle{
        cx - half_w,
        cy - half_h,
        cx + half_w,
        cy + half_h
    });
}

void Build::on_interact(){
    DataCenter* DC = DataCenter::get_instance();
    if(!DC->ui->is_open()) DC->ui->open(this); // 若 UI 尚未開啟，將此 Build 傳入並開啟 UI 視窗
    else DC->ui->close();
}

void Build::child_update(){

}

void Build::draw_ui(UI* ui, float x, float y, float w, float h){

}

void Build::child_init(){
    
}

void Build::update_ui(UI* ui){

}
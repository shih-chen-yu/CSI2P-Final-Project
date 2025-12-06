#ifndef BUILDING_H_INCLUDED
#define BUILDING_H_INCLUDED
#include "../shapes/Point.h"
#include <string>
#include "../Object.h"
#include "../shapes/Rectangle.h"
#include <allegro5/bitmap.h>


namespace BuildingSetting {
    constexpr char normal_img_path[] = "./assets/image/building/buildingA.png";
    constexpr char alert_img_path[]  = "./assets/image/building/buildingA!.png";

    constexpr double trigger_interval_min = 5.0; // 至少隔 5 秒才有機會再觸發
    constexpr double trigger_chance_per_update = 0.01; // 每次 update 有 1% 機率觸發
    constexpr double trigger_radius = 35.0; // 判定玩家「走到」建築的距離（像素）
    constexpr double draw_scale = 0.1; // ★ 圖片縮放倍率，0.5 = 縮小一半
}

class Building : public Object{
public:
    Building(const Point &p);
    void init();
    void update();          // 處理隨機觸發 + 判斷玩家是否走到
    void draw();            // 把正確的貼圖畫出來
    // ★ 讓別人知道這棟有沒有驚嘆號
    bool is_alert() const { return has_event; }

    // ★ 提供中心位置（怪物要往這裡走）
    Point center() const { return pos; }
private:
    Point pos;              // 建築中心位置
    ALLEGRO_BITMAP *normal; // buildingA.png
    ALLEGRO_BITMAP *alert;  // buildingA!.png
    bool has_event;         // true = 有驚嘆號事件
    double trigger_cd;      // 用來控制隨機觸發的冷卻時間（秒）

    Rectangle get_region() const; // 畫圖 / 碰撞用的區域
};
#endif
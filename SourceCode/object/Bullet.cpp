#include "Bullet.h"
#include "../data/DataCenter.h"
#include "../data/ImageCenter.h"    // 假設你有類似的東西，沒有的話暫時用 al_draw_bitmap
#include <allegro5/allegro_primitives.h>
#include <cmath>

void Bullet::init(double sx, double sy, double tx, double ty) {
    // 先設定一個固定大小的 hitbox
    double w = 16.0f;
    double h = 16.0f;

    shape.reset(new Rectangle{
        sx - w / 2.0f,
        sy - h / 2.0f,
        sx + w / 2.0f,
        sy + h / 2.0f
    });

    // 計算方向向量 (tx, ty) - (sx, sy)
    double dx = static_cast<double>(tx - sx);
    double dy = static_cast<double>(ty - sy);
    double len = std::sqrt(dx * dx + dy * dy);
    if (len == 0.0) {
        vx = 0.0;
        vy = -1.0; // 避免除以 0，隨便給一個方向（往上）
    } else {
        vx = dx / len;
        vy = dy / len;
    }
}

void Bullet::update() {
    if (dead) return;

    // 依照方向與速度移動子彈
    double cx = shape->center_x();
    double cy = shape->center_y();

    cx += vx * speed;
    cy += vy * speed;

    shape->update_center_x(cx);
    shape->update_center_y(cy);

    // 飛出視窗就標記為 dead
    DataCenter* DC = DataCenter::get_instance();

    float left   = (float)DC->camera_x;
    float top    = (float)DC->camera_y;
    float right  = left + DC->window_width;
    float bottom = top  + DC->window_height;

    // 多留一點 buffer，避免子彈剛好在邊界就消失
    float margin = 100.0f;

    if (cx < left - margin || cx > right + margin ||
        cy < top  - margin || cy > bottom + margin) {
        dead = true;
    }
}

void Bullet::draw() {
    if (dead) return;

    ImageCenter* IC = ImageCenter::get_instance();
    ALLEGRO_BITMAP* bmp = IC->get("./assets/image/bullets.png");
    if (!bmp) return;

    const float target_w = 25.0f;
    const float target_h = 25.0f;

    float cx = (float)shape->center_x();
    float cy = (float)shape->center_y();

    float dx = cx - target_w / 2.0f;
    float dy = cy - target_h / 2.0f;

    float sw = (float)al_get_bitmap_width(bmp);
    float sh = (float)al_get_bitmap_height(bmp);

    al_draw_scaled_bitmap(
        bmp,
        0, 0, sw, sh,      // 來源整張圖
        dx, dy, target_w, target_h, // 目的：50x50
        0
    );
}
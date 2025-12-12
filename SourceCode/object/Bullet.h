#ifndef BULLET_H_INCLUDED
#define BULLET_H_INCLUDED

#include "../Object.h"
#include "../shapes/Rectangle.h"

class Bullet : public Object {
public:
    // 從 (sx, sy) 朝向 (tx, ty) 射出一顆子彈
    void init(double sx, double sy, double tx, double ty);

    void update();
    void draw() override;

    bool is_dead() const { return dead; }
    void kill() { dead = true; }

private:
    double vx = 0.0;   // 單位方向向量 x
    double vy = 0.0;   // 單位方向向量 y
    double speed = 6.0; // 每次 update 移動多少像素

    bool dead = false; // 飛出畫面或之後被打中就設成 true

    // 這裡直接用 Rectangle 當 hitbox & 繪圖位置
    // Object 裡原本應該有 unique_ptr<Shape> shape;
};

#endif

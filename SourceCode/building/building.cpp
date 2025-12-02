#include "Building.h"
#include "../data/ImageCenter.h"
#include "../data/DataCenter.h"
#include "../Hero.h"
#include "../monsters/Monster.h"
#include <allegro5/bitmap_draw.h>
#include <allegro5/allegro.h>
#include <cstdlib>  // rand
#include <cmath>    // sqrt

Building::Building(const Point &p)
    : pos(p), has_event(false), trigger_cd(0.0)
{
    ImageCenter *IC = ImageCenter::get_instance();
    normal = IC->get(BuildingSetting::normal_img_path);
    alert  = IC->get(BuildingSetting::alert_img_path);
}

void Building::init() {
    // 目前建構子已經把圖跟狀態都設好了
    // 如果之後你想重置狀態，可以寫：
    has_event  = false;
    trigger_cd = 0.0;
}

Rectangle Building::get_region() const {
    int w = al_get_bitmap_width(normal);
    int h = al_get_bitmap_height(normal);

    double sw = w * BuildingSetting::draw_scale;
    double sh = h * BuildingSetting::draw_scale;

    return {
        pos.x,         // 左
        pos.y - sh,    // 上（向下對齊）
        pos.x + sw,    // 右
        pos.y          // 下
    };
}

void Building::update() {
    DataCenter *DC = DataCenter::get_instance();

    // 每幀增加冷卻：1/FPS 秒
    trigger_cd += 1.0 / DC->FPS;

    // 1. 隨機觸發事件（讓 building 變成有驚嘆號）
    if (!has_event && trigger_cd >= BuildingSetting::trigger_interval_min) {
        double r = static_cast<double>(rand()) / RAND_MAX;
        if (r < BuildingSetting::trigger_chance_per_update) {
            has_event = true;
            trigger_cd = 0.0; // 重置冷卻
        }
    }

    // 2. 如果目前有事件 → 檢查「英雄」和「所有怪物」
    if (has_event) {
        // 2-1. 先檢查 Hero
        if (DC->hero) {
            double hx = DC->hero->shape->center_x();
            double hy = DC->hero->shape->center_y();

            double dx = hx - pos.x;
            double dy = hy - pos.y;
            double dist = std::sqrt(dx * dx + dy * dy);

            if (dist <= BuildingSetting::trigger_radius) {
                has_event = false;
                return;  // 已解除，這幀不用再檢查怪物
            }
        }

        // 2-2. 再檢查所有 Monster
        for (Monster *m : DC->monsters) {
            double mx = m->shape->center_x();
            double my = m->shape->center_y();

            double dx = mx - pos.x;
            double dy = my - pos.y;
            double dist = std::sqrt(dx * dx + dy * dy);

            if (dist <= BuildingSetting::trigger_radius) {
                has_event = false; // 任一怪物抵達，也解除驚嘆號
                break;
            }
        }
    }
}

void Building::draw() {
    ALLEGRO_BITMAP *bmp = has_event ? alert : normal;
    int w = al_get_bitmap_width(bmp);
    int h = al_get_bitmap_height(bmp);

    double sw = w * BuildingSetting::draw_scale;
    double sh = h * BuildingSetting::draw_scale;

    // ★ 向左對齊 → x = pos.x
    // ★ 向下對齊 → y = pos.y - sh
    double dx = pos.x;
    double dy = pos.y - sh;

    al_draw_scaled_bitmap(
        bmp,
        0, 0,
        w, h,
        dx, dy,
        sw, sh,
        0
    );
}

#include "Monster.h"
#include "MonsterWolf.h"
#include "MonsterCaveMan.h"
#include "MonsterWolfKnight.h"
#include "MonsterDemonNinja.h"
#include "../data/DataCenter.h"
#include "../data/ImageCenter.h"
#include "../Level.h"
#include "../shapes/Point.h"
#include "../shapes/Rectangle.h"
#include "../Utils.h"
#include "../building/Building.h"
#include <allegro5/allegro_primitives.h>
#include <cmath>
#include <cstdlib>
#include <cstdio>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace std;

// fixed settings
enum class Dir {
    UP, DOWN, LEFT, RIGHT
};

namespace MonsterSetting {
    static constexpr char monster_imgs_root_path[static_cast<int>(MonsterType::MONSTERTYPE_MAX)][40] = {
        "./assets/image/monster/Wolf",
        "./assets/image/monster/CaveMan",
        "./assets/image/monster/WolfKnight",
        "./assets/image/monster/DemonNinja"
    };
    static constexpr char dir_path_prefix[][10] = {
        "UP", "DOWN", "LEFT", "RIGHT"
    };
}

/**
 * @brief Create a Monster* instance by the type.
 */
Monster *Monster::create_monster(MonsterType type, const vector<Point> &path) {
    switch(type) {
        case MonsterType::WOLF: {
            return new MonsterWolf{path};
        }
        case MonsterType::CAVEMAN: {
            return new MonsterCaveMan{path};
        }
        case MonsterType::WOLFKNIGHT: {
            return new MonsterWolfKnight{path};
        }
        case MonsterType::DEMONNIJIA: {
            return new MonsterDemonNinja{path};
        }
        case MonsterType::MONSTERTYPE_MAX: {}
    }
    GAME_ASSERT(false, "monster type error.");
}

/**
 * @brief Given velocity of x and y direction, determine which direction the monster should face.
 */
Dir convert_dir(const Point &v) {
    if(v.y < 0 && std::abs(v.y) >= std::abs(v.x))
        return Dir::UP;
    if(v.y > 0 && std::abs(v.y) >= std::abs(v.x))
        return Dir::DOWN;
    if(v.x < 0 && std::abs(v.x) >= std::abs(v.y))
        return Dir::LEFT;
    if(v.x > 0 && std::abs(v.x) >= std::abs(v.y))
        return Dir::RIGHT;
    return Dir::RIGHT;
}

Monster::Monster(const vector<Point> &path, MonsterType type) {
    DataCenter *DC = DataCenter::get_instance();

    shape.reset(new Rectangle{0, 0, 0, 0});
    this->type = type;
    dir = Dir::RIGHT;
    bitmap_img_id = 0;
    bitmap_switch_counter = 0;
    bitmap_switch_freq = 10;   // 預設值，子類可以覆蓋

    // 只用第一個 path 點決定出生位置
    if(!path.empty()) {
        const Point &grid = path.front();
        const Rectangle &region = DC->level->grid_to_region(grid);
        shape.reset(new Rectangle{
            region.center_x(), region.center_y(),
            region.center_x(), region.center_y()
        });
    }

    // AI 狀態初始化
    ai_state = AIState::WANDER;
    vx = vy = 0.0;
    wander_timer = 0.0;
    target_building = nullptr;
}

/**
 * @brief 選一個新的亂走方向
 */
void Monster::choose_random_direction() {
    double angle = (static_cast<double>(std::rand()) / RAND_MAX) * 2.0 * M_PI;
    double speed = static_cast<double>(v); // v: 像素/秒

    vx = std::cos(angle) * speed;
    vy = std::sin(angle) * speed;

    // 亂走 1 ~ 3 秒後再換方向
    wander_timer = 1.0 + (static_cast<double>(std::rand()) / RAND_MAX) * 2.0;
}

/**
 * @details
 * 1. 處理動畫 frame 切換（有做防呆，避免 frame index 爆掉）
 * 2. 判斷是否有任一棟 building 在冒驚嘆號：
 *    - 有：GO_TO_BUILDING，往那棟 building center 移動
 *    - 沒：WANDER，隨機亂走，碰邊界反彈
 * 3. 更新 hit box 大小
 */
void Monster::update() {
    DataCenter *DC = DataCenter::get_instance();
    ImageCenter *IC = ImageCenter::get_instance();

    double fps = DC->FPS;
    double cx = shape->center_x();
    double cy = shape->center_y();

    // ===== 1. 動畫 frame 切換（保護 bitmap_img_ids 範圍） =====
    int dir_idx = static_cast<int>(dir);

    // 確保 bitmap_img_ids 至少有 4 個方向（避免 out-of-range）
    if (bitmap_img_ids.size() < 4) {
        bitmap_img_ids.resize(4);
    }

    auto &frames = bitmap_img_ids[dir_idx];
    if (!frames.empty()) {
        if (bitmap_switch_counter) {
            --bitmap_switch_counter;
        } else {
            bitmap_img_id = (bitmap_img_id + 1) % static_cast<int>(frames.size());
            bitmap_switch_counter = bitmap_switch_freq;
        }
    } else {
        // 這個方向沒有任何 frame，重設為 0，避免亂 index
        bitmap_img_id = 0;
        bitmap_switch_counter = bitmap_switch_freq;
    }

    // ===== 2. 找出是否有建築在冒驚嘆號 =====
    Building *alert_building = nullptr;
    for (Building *b : DC->Buildings) {
        if (b && b->is_alert()) {
            alert_building = b;
            break;
        }
    }

    // AI 狀態轉換
    if (alert_building) {
        ai_state = AIState::GO_TO_BUILDING;
        target_building = alert_building;
    } else if (ai_state == AIState::GO_TO_BUILDING) {
        ai_state = AIState::WANDER;
        target_building = nullptr;
        wander_timer = 0.0;
    }

    // 一幀理論上能走的距離
    double step = static_cast<double>(v) / fps;

    // ===== 3. 根據 AI 狀態決定移動 =====
    if (ai_state == AIState::GO_TO_BUILDING && target_building) {
        // 往建築物中心走
        Point target = target_building->center();
        double dx = target.x - cx;
        double dy = target.y - cy;
        double dist = std::sqrt(dx * dx + dy * dy);

        if (dist > 1e-3) {
            if (dist <= step) {
                cx = target.x;
                cy = target.y;
            } else {
                cx += dx / dist * step;
                cy += dy / dist * step;
            }
            dir = convert_dir(Point{dx, dy});
        }
    } else {
        // ===== 隨機亂走 =====
        if (wander_timer <= 0.0 || (vx == 0.0 && vy == 0.0)) {
            choose_random_direction();
        } else {
            wander_timer -= 1.0 / fps;
        }

        cx += vx / fps;
        cy += vy / fps;

        // 邊界檢查：不要走出遊戲區域
        double min_x = 0.0;
        double max_x = static_cast<double>(DC->game_field_length);
        double min_y = 0.0;
        double max_y = static_cast<double>(DC->game_field_length);

        if (cx < min_x) { cx = min_x; vx = -vx; }
        if (cx > max_x) { cx = max_x; vx = -vx; }
        if (cy < min_y) { cy = min_y; vy = -vy; }
        if (cy > max_y) { cy = max_y; vy = -vy; }

        dir = convert_dir(Point{vx, vy});
    }

    // 更新中心到 shape
    shape->update_center_x(cx);
    shape->update_center_y(cy);

    // ===== 4. 更新 hit box & 圖片 frame id（再防呆一次） =====
    // 重新抓對應方向的 frames
    auto &frames2 = bitmap_img_ids[dir_idx];
    int frame_id = 0;
    if (!frames2.empty()) {
        if (bitmap_img_id < 0 || bitmap_img_id >= static_cast<int>(frames2.size())) {
            bitmap_img_id = 0;
        }
        frame_id = frames2[bitmap_img_id];
    } else {
        frame_id = 0; // 該方向沒設定圖，就 fallback 到 *_0.png
    }

    char buffer[128];
    std::snprintf(
        buffer, sizeof(buffer), "%s/%s_%d.png",
        MonsterSetting::monster_imgs_root_path[static_cast<int>(type)],
        MonsterSetting::dir_path_prefix[dir_idx],
        frame_id);

    ALLEGRO_BITMAP *bitmap = IC->get(buffer);
    const double &hc = shape->center_x();
    const double &vc = shape->center_y();
    // hitbox 比圖片小一點
    const int h = al_get_bitmap_width(bitmap) * 0.8;
    const int w = al_get_bitmap_height(bitmap) * 0.8;
    shape.reset(new Rectangle{
        (hc - w / 2.), (vc - h / 2.),
        (hc - w / 2. + w), (vc - h / 2. + h)
    });
}

void Monster::draw() {
    ImageCenter *IC = ImageCenter::get_instance();

    int dir_idx = static_cast<int>(dir);
    if (bitmap_img_ids.size() < 4) {
        bitmap_img_ids.resize(4);
    }
    auto &frames = bitmap_img_ids[dir_idx];
    int frame_id = 0;
    if (!frames.empty()) {
        if (bitmap_img_id < 0 || bitmap_img_id >= static_cast<int>(frames.size())) {
            bitmap_img_id = 0;
        }
        frame_id = frames[bitmap_img_id];
    } else {
        frame_id = 0;
    }

    char buffer[128];
    std::snprintf(
        buffer, sizeof(buffer), "%s/%s_%d.png",
        MonsterSetting::monster_imgs_root_path[static_cast<int>(type)],
        MonsterSetting::dir_path_prefix[dir_idx],
        frame_id);

    ALLEGRO_BITMAP *bitmap = IC->get(buffer);
    al_draw_bitmap(
        bitmap,
        shape->center_x() - al_get_bitmap_width(bitmap) / 2,
        shape->center_y() - al_get_bitmap_height(bitmap) / 2,
        0);
}

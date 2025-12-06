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
//#include "../building/Building.h"
#include "../object/Build.h"
#include <allegro5/allegro_primitives.h>
#include <cmath>
#include <cstdlib>
#include <cstdio>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
constexpr double MONSTER_SPEED_SCALE = 0.5; // 比原本慢 50%，想更慢就再調小
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

    
    // === 修改開始：更大幅度的分散 ===
    if(!path.empty()) {
        // 方法：隨機選取「路徑上前 6 格」的其中一格作為出生點
        // 這樣怪物就會沿著路散開，而不是擠在同一個像素
        int range = 6; 
        if(path.size() < 6) range = path.size(); // 防止路徑太短爆掉
        
        int random_index = rand() % range; // 隨機選第 0 ~ 5 格
        const Point &grid = path[random_index]; 
        
        const Rectangle &region = DC->level->grid_to_region(grid);

        // 在那一格裡面，再做一點點微小的亂數偏移 (例如 -15 ~ 15)
        // 這樣就不會死板板的剛好都在格子正中間
        int pixel_offset_x = (rand() % 30) - 15;
        int pixel_offset_y = (rand() % 30) - 15;

        shape.reset(new Rectangle{
            region.center_x() + pixel_offset_x, 
            region.center_y() + pixel_offset_y,
            region.center_x() + pixel_offset_x, 
            region.center_y() + pixel_offset_y
        });
    }

    // AI 狀態初始化
    ai_state = AIState::WANDER;
    vx = vy = 0.0;
    wander_timer = 0.0;
    target_building = nullptr;
    chase_phase = 0;
}

/**
 * @brief 選一個新的亂走方向
 */
void Monster::choose_random_direction() {
    // 讓實際速度比 v 小一點，整體變慢
    double speed = static_cast<double>(v) * MONSTER_SPEED_SCALE; // v: 像素/秒

    // 隨機選一個「只走上下左右」的方向
    int d = std::rand() % 4;
    switch (d) {
        case 0: // 右
            vx =  speed;
            vy =  0.0;
            break;
        case 1: // 左
            vx = -speed;
            vy =  0.0;
            break;
        case 2: // 下
            vx =  0.0;
            vy =  speed;
            break;
        case 3: // 上
        default:
            vx =  0.0;
            vy = -speed;
            break;
    }

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
        bitmap_img_id = 0;
        bitmap_switch_counter = bitmap_switch_freq;
    }

    // ===== 2. 找出所有「正在冒驚嘆號」的建築 =====
    std::vector<Build*> alert_buildings;
    for (Build *b : DC->build) {
        if (b && b->is_alert()) {
            alert_buildings.push_back(b);
        }
    }

    // ===== 2-1. 決定這一幀的目標建築 =====
    // A. 已經有目標，且那棟建築還在冒驚嘆號 → 繼續追，不換目標
    if (target_building && target_building->is_alert()) {
        ai_state = AIState::GO_TO_BUILDING;
    }
    // B. 原本沒有目標，或原本目標不再冒驚嘆號 → 重新決定
    else {
        target_building = nullptr;

        if (!alert_buildings.empty()) {
            // 從有事件的建築中，隨機挑一棟
            int idx = std::rand() % static_cast<int>(alert_buildings.size());
            target_building = alert_buildings[idx];

            ai_state = AIState::GO_TO_BUILDING;
            chase_phase = 0;      // 重新從「先對齊 X」開始追
        } else {
            // 完全沒建築在冒驚嘆號 → 回到亂走
            if (ai_state == AIState::GO_TO_BUILDING) {
                ai_state = AIState::WANDER;
                wander_timer = 0.0;
            }
        }
    }

    // ===== 3. 根據 AI 狀態決定移動 =====
    double step = static_cast<double>(v) * MONSTER_SPEED_SCALE / fps;

    if (ai_state == AIState::GO_TO_BUILDING && target_building) {
        // 追 building：先對齊 X，再對齊 Y
        Point target = target_building->center();
        double dx = target.x - cx;
        double dy = target.y - cy;

        const double eps = 1.0; // 認定「對齊」的誤差

        if (chase_phase == 0) {
            // Phase 0：調整 X
            if (std::fabs(dx) > eps) {
                double move = std::min(step, std::fabs(dx));
                cx += (dx > 0 ? move : -move);
                dir = convert_dir(Point{ (dx > 0 ? 1.0 : -1.0), 0.0 });
            } else {
                chase_phase = 1; // X 對齊後進入調整 Y
            }
        }

        if (chase_phase == 1) {
            // Phase 1：調整 Y
            if (std::fabs(dy) > eps) {
                double move = std::min(step, std::fabs(dy));
                cy += (dy > 0 ? move : -move);
                dir = convert_dir(Point{ 0.0, (dy > 0 ? 1.0 : -1.0) });
            } else {
                // X 和 Y 都差不多對齊 → 怪就停在建築附近
                // 不切回 WANDER，維持在 building 附近等事件消失
            }
        }
    } else {
        // ===== 亂走模式 =====
        if (wander_timer <= 0.0 || (vx == 0.0 && vy == 0.0)) {
            choose_random_direction();
        } else {
            wander_timer -= 1.0 / fps;
        }

        cx += vx / fps;
        cy += vy / fps;

        // 邊界檢查（限制在 game_field 區域內）
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

    // ===== 4. 更新 hit box & 圖片 frame id =====
    auto &frames2 = bitmap_img_ids[dir_idx];
    int frame_id = 0;
    if (!frames2.empty()) {
        if (bitmap_img_id < 0 || bitmap_img_id >= static_cast<int>(frames2.size())) {
            bitmap_img_id = 0;
        }
        frame_id = frames2[bitmap_img_id];
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
    const double &hc = shape->center_x();
    const double &vc = shape->center_y();
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

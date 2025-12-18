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

#include "../object/Build.h"
#include "../Build_object/Build_A.h"
#include "../Build_object/Build_B.h"

#include <allegro5/allegro_primitives.h>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

constexpr double MONSTER_SPEED_SCALE = 0.5;

using namespace std;

enum class Dir { UP, DOWN, LEFT, RIGHT };

namespace MonsterSetting {
    static constexpr char monster_imgs_root_path[static_cast<int>(MonsterType::MONSTERTYPE_MAX)][40] = {
        "./assets/image/monster/Wolf",
        "./assets/image/monster/CaveMan",
        "./assets/image/monster/WolfKnight",
        "./assets/image/monster/DemonNinja"
    };
    static constexpr char dir_path_prefix[][10] = { "UP", "DOWN", "LEFT", "RIGHT" };
}

static Dir convert_dir(const Point &v) {
    if (v.y < 0 && std::abs(v.y) >= std::abs(v.x)) return Dir::UP;
    if (v.y > 0 && std::abs(v.y) >= std::abs(v.x)) return Dir::DOWN;
    if (v.x < 0 && std::abs(v.x) >= std::abs(v.y)) return Dir::LEFT;
    if (v.x > 0 && std::abs(v.x) >= std::abs(v.y)) return Dir::RIGHT;
    return Dir::RIGHT;
}

// ---------------- create ----------------
Monster *Monster::create_monster(MonsterType type, const vector<Point> &path) {
    switch(type) {
        case MonsterType::WOLF:       return new MonsterWolf{path};
        case MonsterType::CAVEMAN:    return new MonsterCaveMan{path};
        case MonsterType::WOLFKNIGHT: return new MonsterWolfKnight{path};
        case MonsterType::DEMONNIJIA: return new MonsterDemonNinja{path};
        case MonsterType::MONSTERTYPE_MAX: break;
    }
    GAME_ASSERT(false, "monster type error.");
    return nullptr;
}

// ---------------- ctor ----------------
Monster::Monster(const vector<Point> &path, MonsterType type) {
    DataCenter *DC = DataCenter::get_instance();

    shape.reset(new Rectangle{0, 0, 0, 0});
    this->type = type;
    dir = Dir::RIGHT;

    bitmap_img_id = 0;
    bitmap_switch_counter = 0;
    bitmap_switch_freq = 10;

    // spawn near early path nodes
    if(!path.empty()) {
        int range = 6;
        if(static_cast<int>(path.size()) < range) range = static_cast<int>(path.size());
        int random_index = rand() % range;
        const Point &grid = path[random_index];
        const Rectangle &region = DC->level->grid_to_region(grid);

        int pixel_offset_x = (rand() % 30) - 15;
        int pixel_offset_y = (rand() % 30) - 15;

        shape.reset(new Rectangle{
            region.center_x() + pixel_offset_x,
            region.center_y() + pixel_offset_y,
            region.center_x() + pixel_offset_x,
            region.center_y() + pixel_offset_y
        });
        spawn_x = shape->center_x();
        spawn_y = shape->center_y();
    }

    ai_state = AIState::WANDER;
    vx = vy = 0.0;
    wander_timer = 0.0;

    target_building = nullptr;     // Build_A*
    target_building_B = nullptr;   // Build_B*
    chase_phase = 0;

    alive = true;
    respawn_timer = 0.0;
}

// ---------------- wander helper ----------------
void Monster::choose_random_direction() {
    double speed = static_cast<double>(v) * MONSTER_SPEED_SCALE;

    int d = std::rand() % 4;
    switch (d) {
        case 0: vx =  speed; vy = 0.0;  break;
        case 1: vx = -speed; vy = 0.0;  break;
        case 2: vx = 0.0;    vy = speed;break;
        default:vx = 0.0;    vy =-speed;break;
    }
    wander_timer = 1.0 + (static_cast<double>(std::rand()) / RAND_MAX) * 2.0;
}

// ---------------- update ----------------
void Monster::update() {
    DataCenter *DC = DataCenter::get_instance();
    ImageCenter *IC = ImageCenter::get_instance();

    double fps = DC->FPS;
    double cx = shape->center_x();
    double cy = shape->center_y();

    // ===== dead state =====
    if (!alive) {
        respawn_timer -= 1.0 / fps;
        if (respawn_timer <= 0.0) {
            alive = true;
            respawn_timer = 0.0;

            shape->update_center_x(spawn_x);
            shape->update_center_y(spawn_y);

            ai_state = AIState::WANDER;
            vx = vy = 0.0;
            wander_timer = 0.0;
            chase_phase = 0;

            target_building = nullptr;
            target_building_B = nullptr;
        }
        return;
    }

    if (bitmap_img_ids.size() < 4) bitmap_img_ids.resize(4);

    // ===== animation tick (by current dir) =====
    {
        int anim_dir_idx = static_cast<int>(dir);
        auto &anim_frames = bitmap_img_ids[anim_dir_idx];

        if (!anim_frames.empty()) {
            if (bitmap_switch_counter > 0) --bitmap_switch_counter;
            else {
                bitmap_img_id = (bitmap_img_id + 1) % static_cast<int>(anim_frames.size());
                bitmap_switch_counter = bitmap_switch_freq;
            }
        } else {
            bitmap_img_id = 0;
            bitmap_switch_counter = bitmap_switch_freq;
        }
    }

    // ===== build lists: available targets (has_food && not targeted) =====
    std::vector<Build_A*> foodA;
    std::vector<Build_B*> foodB;

    for (Build *b : DC->build) {
        if (!b) continue;

        if (auto *ba = dynamic_cast<Build_A*>(b)) {
            if (ba->has_food() && !ba->is_targeted()) foodA.push_back(ba);
        } else if (auto *bb = dynamic_cast<Build_B*>(b)) {
            if (bb->has_food() && !bb->is_targeted()) foodB.push_back(bb);
        }
    }

    // ===== validate current target still has food =====
    if (target_building && !target_building->has_food()) {
        target_building->set_targeted(false);
        target_building = nullptr;
    }
    if (target_building_B && !target_building_B->has_food()) {
        target_building_B->set_targeted(false);
        target_building_B = nullptr;
    }

    // if still has a target => GO
    if (target_building || target_building_B) {
        ai_state = AIState::GO_TO_BUILDING;
    }

    // ===== if no target, pick one randomly from A/B pools =====
    if (!target_building && !target_building_B) {
        int total = (int)foodA.size() + (int)foodB.size();
        if (total > 0) {
            int pick = std::rand() % total;
            if (pick < (int)foodA.size()) {
                target_building = foodA[pick];
                target_building->set_targeted(true);
                target_building_B = nullptr;
            } else {
                pick -= (int)foodA.size();
                target_building_B = foodB[pick];
                target_building_B->set_targeted(true);
                target_building = nullptr;
            }
            ai_state = AIState::GO_TO_BUILDING;
            chase_phase = 0;
        } else {
            ai_state = AIState::WANDER;
        }
    }

    // ===== movement =====
    double step = static_cast<double>(v) * MONSTER_SPEED_SCALE / fps;

    if (ai_state == AIState::GO_TO_BUILDING && (target_building || target_building_B)) {
        Point target = target_building ? target_building->center() : target_building_B->center();

        double dx = target.x - cx;
        double dy = target.y - cy;
        const double eps = 1.0;

        // axis-by-axis chase
        if (chase_phase == 0) {
            if (std::fabs(dx) > eps) {
                double move = std::min(step, std::fabs(dx));
                cx += (dx > 0 ? move : -move);
                dir = convert_dir(Point{ (dx > 0 ? 1.0 : -1.0), 0.0 });
            } else chase_phase = 1;
        }

        if (chase_phase == 1) {
            if (std::fabs(dy) > eps) {
                double move = std::min(step, std::fabs(dy));
                cy += (dy > 0 ? move : -move);
                dir = convert_dir(Point{ 0.0, (dy > 0 ? 1.0 : -1.0) });
            } else {
                // ===== arrived: take ONE food =====
                if (target_building) {
                    target_building->take_food(1);
                    target_building->set_targeted(false);
                    target_building = nullptr;
                }
                if (target_building_B) {
                    target_building_B->take_food(1);
                    target_building_B->set_targeted(false);
                    target_building_B = nullptr;
                }
                ai_state = AIState::WANDER;
                chase_phase = 0;
            }
        }
    } else {
        // wander
        if (wander_timer <= 0.0 || (vx == 0.0 && vy == 0.0)) choose_random_direction();
        else wander_timer -= 1.0 / fps;

        cx += vx / fps;
        cy += vy / fps;

        double min_x = 0.0, min_y = 0.0;
        double max_x = (double)DC->game_field_length;
        double max_y = (double)DC->game_field_length;

        if (cx < min_x) { cx = min_x; vx = -vx; }
        if (cx > max_x) { cx = max_x; vx = -vx; }
        if (cy < min_y) { cy = min_y; vy = -vy; }
        if (cy > max_y) { cy = max_y; vy = -vy; }

        dir = convert_dir(Point{vx, vy});
    }

    shape->update_center_x(cx);
    shape->update_center_y(cy);

    // ===== update hitbox from current frame =====
    int dir_idx = static_cast<int>(dir);
    auto &frames2 = bitmap_img_ids[dir_idx];

    int frame_id = 0;
    if (!frames2.empty()) {
        if (bitmap_img_id < 0 || bitmap_img_id >= (int)frames2.size()) bitmap_img_id = 0;
        frame_id = frames2[bitmap_img_id];
    }

    // clamp to 0..3 (avoid _4.png)
    if (frame_id < 0) frame_id = 0;
    if (frame_id > 3) frame_id = 3;

    char buffer[128];
    std::snprintf(buffer, sizeof(buffer), "%s/%s_%d.png",
                  MonsterSetting::monster_imgs_root_path[(int)type],
                  MonsterSetting::dir_path_prefix[dir_idx],
                  frame_id);

    ALLEGRO_BITMAP *bitmap = IC->get(buffer);
    if (bitmap) {
        const double &hc = shape->center_x();
        const double &vc = shape->center_y();
        const int w = (int)(al_get_bitmap_width(bitmap)  * 0.8);
        const int h = (int)(al_get_bitmap_height(bitmap) * 0.8);

        shape.reset(new Rectangle{
            (hc - w / 2.), (vc - h / 2.),
            (hc - w / 2. + w), (vc - h / 2. + h)
        });
    }
}

void Monster::draw() {
    if (!alive) return;

    ImageCenter *IC = ImageCenter::get_instance();
    if (bitmap_img_ids.size() < 4) bitmap_img_ids.resize(4);

    int dir_idx = static_cast<int>(dir);
    auto &frames = bitmap_img_ids[dir_idx];

    int frame_id = 0;
    if (!frames.empty()) {
        if (bitmap_img_id < 0 || bitmap_img_id >= (int)frames.size()) bitmap_img_id = 0;
        frame_id = frames[bitmap_img_id];
    }
    if (frame_id < 0) frame_id = 0;
    if (frame_id > 3) frame_id = 3;

    char buffer[128];
    std::snprintf(buffer, sizeof(buffer), "%s/%s_%d.png",
                  MonsterSetting::monster_imgs_root_path[(int)type],
                  MonsterSetting::dir_path_prefix[dir_idx],
                  frame_id);

    ALLEGRO_BITMAP *bitmap = IC->get(buffer);
    if (!bitmap) return;

    al_draw_bitmap(
        bitmap,
        shape->center_x() - al_get_bitmap_width(bitmap) / 2,
        shape->center_y() - al_get_bitmap_height(bitmap) / 2,
        0
    );
}

// ---------------- hit by bullet ----------------
void Monster::on_hit_by_bullet() {
    if (!alive) return;

    alive = false;
    respawn_timer = 5.0;
    vx = 0.0;
    vy = 0.0;

    // IMPORTANT: release targeting
    if (target_building) {
        target_building->set_targeted(false);
        target_building = nullptr;
    }
    if (target_building_B) {
        target_building_B->set_targeted(false);
        target_building_B = nullptr;
    }

    ai_state = AIState::WANDER;
    wander_timer = 0.0;
    chase_phase = 0;
}

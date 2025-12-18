#include "hero.h"

#include "../data/DataCenter.h"
#include "../data/ImageCenter.h"
#include "../data/SoundCenter.h"
#include "../shapes/Rectangle.h"
#include "../object/ui.h"
#include "../object/Bullet.h"
#include "../Utils.h"
#include <allegro5/allegro_primitives.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>

static constexpr const char* coin_up_sfx   = "./assets/sound/coin.mp3";
static constexpr const char* coin_down_sfx = "./assets/sound/cashier.mp3";

namespace HeroSetting {
    static constexpr const char* hero_imgs_root_path = "./assets/image/hero";
    static constexpr const char* dir_prefix[] = { "LEFT", "RIGHT", "FRONT", "BACK" };
    static constexpr const char* hero_name[] = { "Snorlax", "Wally"};
    static constexpr int HERO_TYPE_MAX = sizeof(hero_name) / sizeof(hero_name[0]);
}

// --- type 선택 ---
void HERO::set_type(int type_index) {
    if (type_index < 0) type_index = 0;
    if (type_index >= HeroSetting::HERO_TYPE_MAX)
        type_index = HeroSetting::HERO_TYPE_MAX - 1;
    hero_type_index = type_index;
}

// --- init: 用 FRONT_0.png 當尺寸初始化 hitbox ---
void HERO::init() {
    DataCenter* DC = DataCenter::get_instance();
    ImageCenter* IC = ImageCenter::get_instance();

    const char* base = HeroSetting::hero_name[hero_type_index];

    int dir_idx = static_cast<int>(State); // LEFT/RIGHT/FRONT/BACK
    if (dir_idx < 0) dir_idx = 0;
    if (dir_idx > 3) dir_idx = 2; // fallback FRONT

    char path[256];
    std::snprintf(path, sizeof(path), "%s/%s/%s_%d.png",
        HeroSetting::hero_imgs_root_path, base, HeroSetting::dir_prefix[dir_idx], 0);

    ALLEGRO_BITMAP* bmp = IC->get(path);
    GAME_ASSERT(bmp, "Hero png not found. Check ./assets/image/hero/<name>/<DIR>_0.png");

    int w = al_get_bitmap_width(bmp);
    int h = al_get_bitmap_height(bmp);

    shape.reset(new Rectangle{
        DC->window_width  / 2 - w / 2,
        DC->window_height / 2 - h / 2,
        DC->window_width  / 2 + w / 2,
        DC->window_height / 2 + h / 2
    });

    bitmap_img_id = 0;
    bitmap_switch_counter = bitmap_switch_freq;
    is_moving = false;
}

// --- draw: monster-style: dir + frame -> ImageCenter get -> draw_bitmap ---
void HERO::draw() {
    DataCenter* DC = DataCenter::get_instance();
    ImageCenter* IC = ImageCenter::get_instance();

    // god mode glow（保留你原本的）
    if (god_mode) {
        float cx = shape->center_x();
        float cy = shape->center_y();

        float pulse = 0.5f + 0.5f * std::sin((float)al_get_time() * 6.0f);
        unsigned char a = (unsigned char)(60 + 80 * pulse);

        al_draw_filled_circle(cx, cy + 10, 26, al_map_rgba(80, 180, 255, a));
        al_draw_filled_circle(cx, cy + 10, 18, al_map_rgba(120, 210, 255, a));
        al_draw_filled_circle(cx, cy + 10, 10, al_map_rgba(180, 240, 255, a));
    }

    const char* base = HeroSetting::hero_name[hero_type_index];

    int dir_idx = static_cast<int>(State);
    if (dir_idx < 0) dir_idx = 0;
    if (dir_idx > 3) dir_idx = 2;

    // 不動就固定第 0 張；移動才播動畫
    int frame_id = is_moving ? bitmap_img_id : 0;

    if (frame_id < 0) frame_id = 0;
    if (frame_id >= frame_count) frame_id = frame_count - 1;

    char path[256];
    std::snprintf(path, sizeof(path), "%s/%s/%s_%d.png",
        HeroSetting::hero_imgs_root_path, base, HeroSetting::dir_prefix[dir_idx], frame_id);

    ALLEGRO_BITMAP* bmp = IC->get(path);
    if (!bmp) return;

    al_draw_bitmap(
        bmp,
        shape->center_x() - al_get_bitmap_width(bmp) / 2.0f,
        shape->center_y() - al_get_bitmap_height(bmp) / 2.0f,
        0
    );
}

// --- update: 保留你原本的射擊/扣飽食/移動，外加動畫 counter ---
void HERO::update() {
    DataCenter* DC = DataCenter::get_instance();

    if (starve > 100) starve = 100;

    // ===== 射擊（保留）=====
    bool space_pressed = DC->key_state[ALLEGRO_KEY_SPACE] && !DC->prev_key_state[ALLEGRO_KEY_SPACE];
    if (space_pressed) {
        auto [hx, hy] = get_position();
        double mx = DC->mouse.x + DC->camera_x;
        double my = DC->mouse.y + DC->camera_y;

        Bullet* b = new Bullet();
        b->init(hx, hy, mx, my);
        DC->bullets.push_back(b);

        if (!god_mode) {
            starve -= 10.0;
        }
    }

    // ===== 移動（保留你的 UI 開啟時不動）=====
    is_moving = false;

    if (!(DC->ui && DC->ui->is_open())) {
        double extra = stamina_extra_speed * (starve / 100.0);

        if (DC->key_state[ALLEGRO_KEY_W]) {
            shape->update_center_y(shape->center_y() - speed - extra);
            State = HeroState::BACK;
            is_moving = true;
            if (!god_mode) starve -= starve_decrease_rate_walk;
        }
        else if (DC->key_state[ALLEGRO_KEY_S]) {
            shape->update_center_y(shape->center_y() + speed + extra);
            State = HeroState::FRONT;
            is_moving = true;
            if (!god_mode) starve -= starve_decrease_rate_walk;
        }
        else if (DC->key_state[ALLEGRO_KEY_A]) {
            shape->update_center_x(shape->center_x() - speed - extra);
            State = HeroState::LEFT;
            is_moving = true;
            if (!god_mode) starve -= starve_decrease_rate_walk;
        }
        else if (DC->key_state[ALLEGRO_KEY_D]) {
            shape->update_center_x(shape->center_x() + speed + extra);
            State = HeroState::RIGHT;
            is_moving = true;
            if (!god_mode) starve -= starve_decrease_rate_walk;
        }
        else {
            if (!god_mode) starve -= starve_decrease_rate;
        }
    }

    // ===== 動畫（monster-style counter）=====
    if (is_moving) {
        if (bitmap_switch_counter > 0) {
            --bitmap_switch_counter;
        } else {
            bitmap_img_id = (bitmap_img_id + 1) % frame_count;  // 0..3
            bitmap_switch_counter = bitmap_switch_freq;
        }
    } else {
        bitmap_img_id = 0;
        bitmap_switch_counter = bitmap_switch_freq;
    }
}

// ===== deposit 音效（保留你原本的）=====
void HERO::add_deposit(int reward) {
    if (reward <= 0) return;
    deposit += reward;

    if (auto* SC = SoundCenter::get_instance()) {
        SC->play(coin_up_sfx, ALLEGRO_PLAYMODE_ONCE);
    }
}

void HERO::reduce_deposit(int cost) {
    if (cost <= 0) return;

    int before = (int)deposit;
    deposit -= cost;
    if (deposit < 0) deposit = 0;

    if ((int)deposit != before) {
        if (auto* SC = SoundCenter::get_instance()) {
            SC->play(coin_down_sfx, ALLEGRO_PLAYMODE_ONCE);
        }
    }
}

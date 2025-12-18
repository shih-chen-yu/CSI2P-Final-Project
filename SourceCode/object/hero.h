#ifndef HERO_H_INCLUDED
#define HERO_H_INCLUDED

#include <string>
#include <utility>
#include "../Object.h"

enum class HeroState{
    LEFT,
    RIGHT,
    FRONT,
    BACK,
    HEROSTATE_MAX
};

class HERO : public Object{
public:
    void init();
    void update();
    void draw() override;

    double get_starve() const { return starve; }
    double get_deposit() const { return deposit; }

    void add_stamina(double stamina) { starve += stamina; }
    void add_deposit(int reward);
    void reduce_deposit(int cost);
    bool can_afford(int cost) const { return deposit >= cost; }

    void set_type(int type_index);

    std::pair<float, float> get_position() const {
        return {shape->center_x(), shape->center_y()};
    }

    void set_god_mode(bool on) { god_mode = on; }
    bool is_god_mode() const { return god_mode; }

private:
    // ===== movement / status =====
    HeroState State = HeroState::FRONT;
    double speed = 1.5;
    double stamina_extra_speed = 1.5;

    double starve = 100;
    double starve_decrease_rate = 0.01;      // 每次 update 減少的飢餓值（站著）
    double starve_decrease_rate_walk = 0.05; // 有走路狀態下 update 減少的飢餓值

    // merge1 的初始值：避免一開局就變成 debug 超大錢
    double deposit = 50;

    int hero_type_index = 0;
    bool god_mode = false;

    // ===== PNG animation (monster-style) =====
    int bitmap_img_id = 0;            // 0..(frame_count-1)
    int bitmap_switch_counter = 0;
    int bitmap_switch_freq = 8;       // 越小動畫越快
    bool is_moving = false;           // update() 內決定
    int frame_count = 4;              // 你若只有 _0~_3.png 就是 4
};

#endif

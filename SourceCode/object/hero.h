#ifndef HERO_H_INCLUDED
#define HERO_H_INCLUDED

#include<string>
#include<map>
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
        int get_score() const { return score; }
        void add_score(int s) { score += s; }
    private:
        HeroState State = HeroState::FRONT;
        double speed = 1.5;
        double stamina_extra_speed = 1.5;
        double starve = 100;
        double starve_decrease_rate = 0.01; // 每次 update 減少的飢餓值
        double starve_decrease_rate_walk = 0.05; // 如果有走路的狀態下 update減少的飢餓值
        double deposit = 50;
        int score = 0;
        std::map<HeroState, std::string> gifPath;
        int hero_type_index = 0;
        bool god_mode = false;
};

#endif
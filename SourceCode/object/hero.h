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
        void reduce_deposit(int cost) { deposit -= cost; }
    private:
        HeroState State = HeroState::FRONT;
        double speed = 2;
        double starve = 100;
        double starve_decrease_rate = 0.01; // 每次 update 減少的飢餓值
        double starve_decrease_rate_walk = 0.05; // 如果有走路的狀態下 update減少的飢餓值
        double deposit = 114514;
        std::map<HeroState, std::string> gifPath;
};

#endif
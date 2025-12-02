#include "DataCenter.h"
#include <cstring>
#include "../Level.h"
#include "../Player.h"
#include "../monsters/Monster.h"
#include "../towers/Tower.h"
#include "../towers/Bullet.h"   // 如果之後完全不用 Bullet，可以一起移掉
#include "../Hero.h"
#include "../building/Building.h"

// fixed settings
namespace DataSetting {
    constexpr double FPS = 60;
    constexpr int window_width = 800;
    constexpr int window_height = 600;
    constexpr int game_field_length = 600;
}

DataCenter::DataCenter() {
    this->FPS = DataSetting::FPS;
    this->window_width = DataSetting::window_width;
    this->window_height = DataSetting::window_height;
    this->game_field_length = DataSetting::game_field_length;

    memset(key_state, false, sizeof(key_state));
    memset(prev_key_state, false, sizeof(prev_key_state));
    mouse = Point(0, 0);
    memset(mouse_state, false, sizeof(mouse_state));
    memset(prev_mouse_state, false, sizeof(prev_mouse_state));

    player   = new Player();
    level    = new Level();
    hero     = new Hero();
    
}

DataCenter::~DataCenter() {
    delete player;
    delete level;
    delete hero;

    // 刪掉所有怪物
    for (Monster *&m : monsters) {
        delete m;
    }

    // 刪掉所有塔
    for (Tower *&t : towers) {
        delete t;
    }

    // 如果你已經不再使用 Bullet，可以把下面這段也移除，順便從 DataCenter.h 刪掉 towerBullets
    for (Bullet *&tb : towerBullets) {
        delete tb;
    }

    // 單一 building 指標，直接 delete 就好
    for (Building *b : Buildings) {
        delete b;
    }

}

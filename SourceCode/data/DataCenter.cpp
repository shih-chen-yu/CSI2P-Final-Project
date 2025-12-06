#include "DataCenter.h"
#include <cstring>
#include "../Level.h"
#include "../Player.h"
#include "../monsters/Monster.h"

#include "../object/hero.h"
#include "../object/Build.h"
#include "../object/ui.h"
#include "../object/Phone.h"
#include "../Map.h"

#include "../info/StarveInfo.h"
#include "../info/CoinInfo.h"

// fixed settings
namespace DataSetting {
    constexpr double FPS = 60;
    constexpr int window_width = 800;
    constexpr int window_height = 600;
    constexpr int game_field_length = 800;
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
    hero = new HERO();

	ui = new UI();
	map = new Map();
	phone = new Phone();

	starve_info = new StarveInfo();
	coin_info = new CoinInfo();
    
}

DataCenter::~DataCenter() {
    delete level;
    if(player) delete player;
	if(hero) delete hero;
	for(auto b : build) delete b;
    if(ui) delete ui;
    if(map) delete map;
	if(phone) delete phone;
	if(starve_info) delete starve_info;
	if(coin_info) delete coin_info;

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
}

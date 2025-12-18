#include "DataCenter.h"
#include <cstring>

#include "../Level.h"
#include "../Player.h"
#include "../LevelTimer.h"
#include "../monsters/Monster.h"

#include "../object/hero.h"
#include "../object/Build.h"
#include "../object/ui.h"
#include "../object/Phone.h"
#include "../Map.h"

#include "../info/StarveInfo.h"
#include "../info/CoinInfo.h"
#include "../info/TimeInfo.h"
#include "../info/ScoreInfo.h"

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
    this->camera_x = 0.0;
    this->camera_y = 0.0;
    camera_shake_timer = 0.0f;    
    camera_shake_strength = 0.0f;
    memset(key_state, false, sizeof(key_state));
    memset(prev_key_state, false, sizeof(prev_key_state));
    mouse = Point(0, 0);
    memset(mouse_state, false, sizeof(mouse_state));
    memset(prev_mouse_state, false, sizeof(prev_mouse_state));

    level = new Level();
    leveltimer = new LevelTimer();
    hero = new HERO();
	player = new Player();

	ui = new UI();
	map = new Map();
	phone = new Phone();

	starve_info = new StarveInfo();
	coin_info = new CoinInfo();
    time_info = new TimeInfo();
    score_info = new ScoreInfo();
}

DataCenter::~DataCenter() {
    if(player) delete player;
	if(hero) delete hero;
	for(auto b : build) delete b;
    if(ui) delete ui;
    if(map) delete map;
	if(phone) delete phone;
	if(starve_info) delete starve_info;
	if(coin_info) delete coin_info;
    if(time_info) delete time_info;
}

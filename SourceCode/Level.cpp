#include "Level.h"
#include <string>
#include "Utils.h"
#include "monsters/Monster.h"
#include "data/DataCenter.h"
#include <allegro5/allegro_primitives.h>
#include "shapes/Point.h"
#include "shapes/Rectangle.h"
#include <array>

using namespace std;

// fixed settings
namespace LevelSetting {
	constexpr char level_path_format[] = "./assets/level/LEVEL%d.txt";
	//! @brief Grid size for each level.
	constexpr array<int, 4> grid_size = {
		40, 40, 40, 40
	};
	constexpr int monster_spawn_rate = 90;
};

void
Level::init() {
	level = -1;
	grid_w = -1;
	grid_h = -1;
	monster_spawn_counter = 0;
}

/**
 * @brief Loads level data from input file. The input file is required to follow the format.
 * @param lvl level index. The path format is a fixed setting in code.
 * @details The content of the input file should be formatted as follows:
 *          * Total number of monsters.
 *          * Number of each different number of monsters. The order and number follows the definition of MonsterType.
 *          * Indefinite number of Point (x, y), represented in grid format.
 * @see level_path_format
 * @see MonsterType
 */
void
Level::load_level(int lvl) {
	DataCenter *DC = DataCenter::get_instance();

	char buffer[50];
	sprintf(buffer, LevelSetting::level_path_format, lvl);
	FILE *f = fopen(buffer, "r");
	GAME_ASSERT(f != nullptr, "cannot find level.");
	level = lvl;
	grid_w = DC->game_field_length / LevelSetting::grid_size[lvl];
	grid_h = DC->game_field_length / LevelSetting::grid_size[lvl];
	num_of_monsters.clear();
	road_path.clear();

	int num;
	// read total number of monsters & number of each monsters
	fscanf(f, "%d", &num);
	for(size_t i = 0; i < static_cast<size_t>(MonsterType::MONSTERTYPE_MAX); ++i) {
		fscanf(f, "%d", &num);
		num_of_monsters.emplace_back(num);
	}
	/*
	// read road path
	while(fscanf(f, "%d", &num) != EOF) {
		int w = num % grid_w;
		int h = num / grid_h;
		road_path.emplace_back(w, h);
	}
	debug_log("<Level> load level %d.\n", lvl);
	fclose(f); // 記得關閉檔案

    debug_log("<Level> load level %d.\n", lvl);*/

    // ==========================================
    // 修改處：在這裡直接生成 5 隻怪物
    // ==========================================
    
    // 確保先清空舊的怪物
    DC->monsters.clear();

    int spawned_count = 0;
    int target_spawn_num = 5; // 目標生成 5 隻

    // 遍歷我們從檔案讀到的怪物數量設定
    for(size_t i = 0; i < num_of_monsters.size(); ++i) {
        // 當這種怪物還有剩餘數量，且我們還沒生滿 5 隻時
        while(num_of_monsters[i] > 0 && spawned_count < target_spawn_num) {
            
            // 建立怪物 (使用 Monster::create_monster)
            // 注意：road_path 必須已經讀取完畢
            DC->monsters.emplace_back(
                Monster::create_monster(static_cast<MonsterType>(i), road_path)
            );

            num_of_monsters[i]--; // 扣除剩餘數量
            spawned_count++;      // 增加已生成數量
        }
    }
}

/**
 * @brief Updates monster_spawn_counter and create monster if needed.
*/
void
Level::update() {
	return;
}

void
Level::draw() {
	/*
	if(level == -1) return;
	for(auto &[i, j] : road_path) {
		int x1 = i * LevelSetting::grid_size[level];
		int y1 = j * LevelSetting::grid_size[level];
		int x2 = x1 + LevelSetting::grid_size[level];
		int y2 = y1 + LevelSetting::grid_size[level];
		al_draw_filled_rectangle(x1, y1, x2, y2, al_map_rgb(255, 244, 173));
	}*/
}

/*
bool
Level::is_onroad(const Rectangle &region) {
	for(const Point &grid : road_path) {
		if(grid_to_region(grid).overlap(region))
			return true;
	}
	return false;
}*/


Rectangle
Level::grid_to_region(const Point &grid) const {
	int x1 = grid.x * LevelSetting::grid_size[level];
	int y1 = grid.y * LevelSetting::grid_size[level];
	int x2 = x1 + LevelSetting::grid_size[level];
	int y2 = y1 + LevelSetting::grid_size[level];
	return Rectangle{x1, y1, x2, y2};
}

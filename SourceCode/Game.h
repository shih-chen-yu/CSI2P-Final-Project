#ifndef GAME_H_INCLUDED
#define GAME_H_INCLUDED

#include <allegro5/allegro.h>


struct ALLEGRO_SAMPLE_INSTANCE;
/**
 * @brief Main class that runs the whole game.
 * @details All game procedures must be processed through this class.
 */
class Game
{
public:
	void execute();
public:
	Game(bool testMode = false);
	~Game();
	void game_init();
	bool game_update();
	void game_draw();
private:
	/**
	 * @brief States of the game process in game_update.
	 * @see Game::game_update()
	 */
	enum class STATE {
		START, // 主選單
		HELP,  // 遊戲說明
		UI,    // 選角 / 選關
		LEVEL, // 遊戲主畫面
		PAUSE,
		END
	};
	STATE state;
	ALLEGRO_EVENT event;
	ALLEGRO_BITMAP *game_icon;
	ALLEGRO_BITMAP *background;   // 遊戲中背景（現在用的）
    ALLEGRO_BITMAP *menu_bg;      // 主選單背景
    ALLEGRO_BITMAP *select_bg;    // 角色/關卡選擇背景

	// 背景音樂 & 音量
    ALLEGRO_SAMPLE_INSTANCE *bgm_instance; // BGM 播放實例
    float bgm_volume;                      // 0.0 ~ 1.0 對應 0~100%
private:
	ALLEGRO_DISPLAY *display;
	ALLEGRO_TIMER *timer;
	ALLEGRO_EVENT_QUEUE *event_queue;
	//UI *ui;
	int selected_hero_index = 0;       // Select 畫面正在選的角色
    static constexpr int HERO_TYPE_MAX = 3;  // 要跟 HeroSetting::HERO_TYPE_MAX 一致
};

#endif

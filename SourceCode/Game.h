#ifndef GAME_H_INCLUDED
#define GAME_H_INCLUDED

#include <allegro5/allegro.h>

struct ALLEGRO_SAMPLE_INSTANCE;

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
	enum class STATE {
		START,
		HELP,
		UI,
		LEVEL,
		PAUSE,
		END,
		END_SUCCESS
	};
	STATE state;

	ALLEGRO_EVENT event;
	ALLEGRO_BITMAP *game_icon;
	ALLEGRO_BITMAP *background;
	ALLEGRO_BITMAP *start_bg;
    ALLEGRO_BITMAP *menu_bg;
    ALLEGRO_BITMAP *select_bg;
	ALLEGRO_BITMAP *help_bg;
	ALLEGRO_BITMAP *ui_bg;
	ALLEGRO_BITMAP *skull_img;
	ALLEGRO_BITMAP *start_button;

    ALLEGRO_SAMPLE_INSTANCE *bgm_instance;
    float bgm_volume;

private:
	ALLEGRO_DISPLAY *display;
	ALLEGRO_TIMER *timer;
	ALLEGRO_EVENT_QUEUE *event_queue;

	int selected_hero_index = 0;
    static constexpr int HERO_TYPE_MAX = 2;  // ✅ Snorlax, Wally

	bool  hero_starved;
    bool  game_over_sound_played;
    float game_over_timer;

	bool  game_success_sound_played = false;
	float game_success_timer = 0.0f;

	bool god_mode = false;

	float ui_vol_y = 0.0f;
    float ui_god_y = 0.0f;
    float ui_slider_x1 = 0.0f;
    float ui_slider_x2 = 0.0f;
};

#endif

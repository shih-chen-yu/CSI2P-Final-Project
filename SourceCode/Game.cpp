#include "Game.h"
#include "Utils.h"

#include "data/DataCenter.h"
#include "data/OperationCenter.h"
#include "data/SoundCenter.h"
#include "data/ImageCenter.h"
#include "data/FontCenter.h"

#include "Player.h"
#include "Level.h"
#include "LevelTimer.h"
#include "Map.h"

#include "object/Build.h"
#include "object/hero.h"
#include "object/ui.h"
#include "object/Phone.h"

#include "info/StarveInfo.h"
#include "info/CoinInfo.h"
#include "info/TimeInfo.h" 
#include "info/ScoreInfo.h"

#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>	
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_acodec.h>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <ctime>

namespace {
    constexpr const char* HERO_PREVIEW_ROOT = "./assets/image/hero";
    constexpr const char* HERO_NAMES[] = {
        "Snorlax",
        "Wally"
    };
    constexpr int HERO_TYPE_MAX = sizeof(HERO_NAMES) / sizeof(HERO_NAMES[0]);

    constexpr const char* HERO_DIR_PREFIX = "FRONT"; // 預覽固定正面
    constexpr int HERO_PREVIEW_FRAME_COUNT = 4;      // 0..3
    constexpr int HERO_PREVIEW_FREQ = 6;             // 越小越快

    void draw_fullscreen_bitmap(ALLEGRO_BITMAP* bmp, int win_w, int win_h) {
        if (!bmp) return;
        int bw = al_get_bitmap_width(bmp);
        int bh = al_get_bitmap_height(bmp);
        al_draw_scaled_bitmap(bmp, 0, 0, bw, bh, 0, 0, win_w, win_h, 0);
    }
}


constexpr char game_icon_img_path[] = "./assets/image/game_icon.png";
constexpr char game_start_sound_path[] = "./assets/sound/growl.mp3";
constexpr char menu_img_path[]           = "./assets/image/MenuBackground.png";
constexpr char select_img_path[]         = "./assets/image/SelectBackground.png";
constexpr char background_img_path[]     = "./assets/image/StartBackground.jpg";
constexpr char start_img_path[]          = "./assets/image/start.png";
constexpr char start_button_img_path[]   = "./assets/image/start_button.png";
constexpr char background_sound_path[]   = "./assets/sound/u3krw-qzk20.wav";
constexpr char game_over_sound_path[]    = "./assets/sound/game-over-38511.mp3";
constexpr char game_success_sound_path[] = "./assets/sound/gamepasss.mp3";
constexpr char skull_img_path[]          = "./assets/image/skull.png";
constexpr char help_bg_img_path[]        = "./assets/image/HelpBackground.png";
constexpr char ui_bg_img_path[]          = "./assets/image/HelpBackground.png";

void
Game::execute() {
	DataCenter *DC = DataCenter::get_instance();
	std::srand((unsigned)std::time(nullptr));
	bool run = true;
	while(run) {
		al_wait_for_event(event_queue, &event);
		switch(event.type) {
			case ALLEGRO_EVENT_TIMER: {
				run &= game_update();
				game_draw();
				break;
			} case ALLEGRO_EVENT_DISPLAY_CLOSE: {
				run = false;
				break;
			} case ALLEGRO_EVENT_KEY_DOWN: {
				DC->key_state[event.keyboard.keycode] = true;

				switch (event.keyboard.keycode) {
					case ALLEGRO_KEY_M:
						if(state != STATE::START && state != STATE::PAUSE) DC->phone->toggle();
						break;
					default:
						break;
				}
				break;
			} case ALLEGRO_EVENT_KEY_UP: {
				DC->key_state[event.keyboard.keycode] = false;
				break;
			} case ALLEGRO_EVENT_MOUSE_AXES: {
				DC->mouse.x = event.mouse.x;
				DC->mouse.y = event.mouse.y;
				break;
			} case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN: {
				DC->mouse_state[event.mouse.button] = true;
				break;
			} case ALLEGRO_EVENT_MOUSE_BUTTON_UP: {
				DC->mouse_state[event.mouse.button] = false;
				break;
			} default: break;
		}
	}
}

Game::Game(bool testMode) {
	DataCenter *DC = DataCenter::get_instance();
	GAME_ASSERT(al_init(), "failed to initialize allegro.");

	bool addon_init = true;
	addon_init &= al_init_primitives_addon();
	addon_init &= al_init_font_addon();
	addon_init &= al_init_ttf_addon();
	addon_init &= al_init_image_addon();
	addon_init &= al_init_acodec_addon();
	GAME_ASSERT(addon_init, "failed to initialize allegro addons.");

	if(testMode) {
		timer = nullptr;
		event_queue = nullptr;
		display = nullptr;
		debug_log("Game initialized in test mode.\n");
		return;
	}

	bool event_init = true;
	event_init &= al_install_keyboard();
	event_init &= al_install_mouse();
	event_init &= al_install_audio();
	GAME_ASSERT(event_init, "failed to initialize allegro events.");

	GAME_ASSERT(
		timer = al_create_timer(1.0 / DC->FPS),
		"failed to create timer.");
	GAME_ASSERT(
		event_queue = al_create_event_queue(),
		"failed to create event queue.");
	GAME_ASSERT(
		display = al_create_display(DC->window_width, DC->window_height),
		"failed to create display.");

	debug_log("Game initialized.\n");
	game_init();
}

void
Game::game_init() {
	DataCenter *DC = DataCenter::get_instance();
	SoundCenter *SC = SoundCenter::get_instance();
	ImageCenter *IC = ImageCenter::get_instance();
	FontCenter *FC = FontCenter::get_instance();

	game_icon = IC->get(game_icon_img_path);
	al_set_display_icon(display, game_icon);

	selected_hero_index = 0;

    al_register_event_source(event_queue, al_get_display_event_source(display));
    al_register_event_source(event_queue, al_get_keyboard_event_source());
    al_register_event_source(event_queue, al_get_mouse_event_source());
    al_register_event_source(event_queue, al_get_timer_event_source(timer));

	SC->init();
	FC->init();

	DC->ui->init();
	DC->map->init();
	DC->phone->init();
	DC->hero->init();
	DC->starve_info->init();
	DC->coin_info->init();
	DC->time_info->init();

	menu_bg     = IC->get(menu_img_path);
	select_bg   = IC->get(select_img_path);
	background  = IC->get(background_img_path);
	start_bg    = IC->get(start_img_path);
	start_button= IC->get(start_button_img_path);
	skull_img   = IC->get(skull_img_path);
	help_bg     = IC->get(help_bg_img_path);
	ui_bg       = IC->get(ui_bg_img_path);

	DC->level->init();
	DC->leveltimer->init();

	bgm_instance = nullptr;
	bgm_volume = 0.4f;

	hero_starved = false;
    game_over_sound_played = false;
    game_over_timer = 0.0f;

	game_success_sound_played = false;
	game_success_timer = 0.0f;

	debug_log("Game state: change to START\n");
	state = STATE::START;
	al_start_timer(timer);
}

bool
Game::game_update() {
    DataCenter *DC = DataCenter::get_instance();
    OperationCenter *OC = OperationCenter::get_instance();
    SoundCenter *SC = SoundCenter::get_instance();
    double dt = 1.0 / DC->FPS;

    switch(state) {
        case STATE::START: {
            static bool is_played = false;
            if(!is_played) {
                SC->play(game_start_sound_path, ALLEGRO_PLAYMODE_ONCE);
                is_played = true;
            }

			if(DC->key_state[ALLEGRO_KEY_ENTER] && !DC->prev_key_state[ALLEGRO_KEY_ENTER]) {
				debug_log("<Game> state: change to HELP\n");
				state = STATE::HELP;
			}

            if(DC->key_state[ALLEGRO_KEY_ESCAPE] && !DC->prev_key_state[ALLEGRO_KEY_ESCAPE]) {
                return false;
            }

			float cx = DC->window_width / 2.f;
			float btn_w = 260.0f;
			float btn_h = 70.0f;
			float btn_x = cx - btn_w / 2.0f;
			float btn_y = DC->window_height * 0.7f;

			bool left_now  = DC->mouse_state[1];
			bool left_prev = DC->prev_mouse_state[1];

			if (left_now && !left_prev) {
				int mx = DC->mouse.x;
				int my = DC->mouse.y;

				if (mx >= btn_x && mx <= btn_x + btn_w &&
					my >= btn_y && my <= btn_y + btn_h) {

					debug_log("<Game> START: PLAY button clicked, change to HELP\n");
					state = STATE::HELP;
				}
			}
            break;
        }
		case STATE::HELP: {
			if(DC->key_state[ALLEGRO_KEY_ENTER] && !DC->prev_key_state[ALLEGRO_KEY_ENTER]) {
				debug_log("<Game> state: change to UI (Select)\n");
				state = STATE::UI;
			}

			if(DC->key_state[ALLEGRO_KEY_BACKSPACE] && !DC->prev_key_state[ALLEGRO_KEY_BACKSPACE]) {
				debug_log("<Game> state: back to START\n");
				state = STATE::START;
			}
			break;
		}
        case STATE::LEVEL: {
            if(!bgm_instance) {
                bgm_instance = SC->play(background_sound_path, ALLEGRO_PLAYMODE_LOOP, bgm_volume);
            }

            if(DC->key_state[ALLEGRO_KEY_EQUALS] && !DC->prev_key_state[ALLEGRO_KEY_EQUALS]) {
                bgm_volume += 0.05f;
                if(bgm_volume > 1.0f) bgm_volume = 1.0f;
                if(bgm_instance) SC->set_volume(bgm_instance, bgm_volume);
                debug_log("Volume Up: %f\n", bgm_volume);
            }
            if(DC->key_state[ALLEGRO_KEY_MINUS] && !DC->prev_key_state[ALLEGRO_KEY_MINUS]) {
                bgm_volume -= 0.05f;
                if(bgm_volume < 0.0f) bgm_volume = 0.0f;
                if(bgm_instance) SC->set_volume(bgm_instance, bgm_volume);
                debug_log("Volume Down: %f\n", bgm_volume);
            }

			// ===== merge1 的 debug 熱鍵（只加這兩個，其它照 merge2）=====
			if(DC->key_state[ALLEGRO_KEY_C] && !DC->prev_key_state[ALLEGRO_KEY_C]) {
				DC->hero->add_deposit(100); // +100
			}
			if(DC->key_state[ALLEGRO_KEY_L] && !DC->prev_key_state[ALLEGRO_KEY_L]) {
				DC->leveltimer->set_level(DC->leveltimer->get_level() + 1); // 跳關
			}
			// ============================================================

			DC->leveltimer->update(dt);
			int timer_level = DC->leveltimer->get_level();
			if (timer_level > 4) {
				debug_log("<Game> state: change to END_SUCCESS (CLEAR)\n");

				if (bgm_instance) {
					SC->toggle_playing(bgm_instance);
				}

				game_success_sound_played = false;
				game_success_timer = 0.0f;
				state = STATE::END_SUCCESS;
				break;
			}

            if(DC->key_state[ALLEGRO_KEY_P] && !DC->prev_key_state[ALLEGRO_KEY_P]) {
				debug_log("<Game> state: change to PAUSE\n");
				state = STATE::PAUSE;
			}

            if(!god_mode && DC->hero->get_starve() <= 0.0) {
                debug_log("<Game> state: change to END (STARVED)\n");
				hero_starved = true;
				if (bgm_instance) {
                    SC->toggle_playing(bgm_instance);
                }

				game_over_sound_played = false;
				game_over_timer = 0.0f;
				state = STATE::END;
            }
            break;
        }

        case STATE::UI: {
			if(!bgm_instance) {
				bgm_instance = SC->play(background_sound_path, ALLEGRO_PLAYMODE_LOOP, bgm_volume);
			}

			if(DC->key_state[ALLEGRO_KEY_A] && !DC->prev_key_state[ALLEGRO_KEY_A]) {
				selected_hero_index--;
				if(selected_hero_index < 0) selected_hero_index = HERO_TYPE_MAX - 1;
			}
			if(DC->key_state[ALLEGRO_KEY_D] && !DC->prev_key_state[ALLEGRO_KEY_D]) {
				selected_hero_index++;
				if(selected_hero_index >= HERO_TYPE_MAX) selected_hero_index = 0;
			}

			// ✅ 只用 draw 算好的 hitbox（ui_*）
			if (DC->mouse_state[1]) {
				int mx = DC->mouse.x;
				int my = DC->mouse.y;

				// volume slider
				if (mx >= ui_slider_x1 && mx <= ui_slider_x2 &&
					my >= ui_vol_y - 10 && my <= ui_vol_y + 10) {
					float t = (mx - ui_slider_x1) / (ui_slider_x2 - ui_slider_x1);
					if(t < 0.0f) t = 0.0f;
					if(t > 1.0f) t = 1.0f;
					bgm_volume = t;
					if(bgm_instance) SC->set_volume(bgm_instance, bgm_volume);
				}

				// god slider
				if (mx >= ui_slider_x1 && mx <= ui_slider_x2 &&
					my >= ui_god_y - 10 && my <= ui_god_y + 10) {
					float t = (mx - ui_slider_x1) / (ui_slider_x2 - ui_slider_x1);
					god_mode = (t > 0.5f);
				}
			}

			if (DC->key_state[ALLEGRO_KEY_G] && !DC->prev_key_state[ALLEGRO_KEY_G]) {
				god_mode = !god_mode;
			}

			if(DC->key_state[ALLEGRO_KEY_ENTER] && !DC->prev_key_state[ALLEGRO_KEY_ENTER]) {
				DC->hero->set_type(selected_hero_index);
				DC->hero->init();
				DC->hero->set_god_mode(god_mode);
				DC->level->load_level(1);
				state = STATE::LEVEL;
			}

			if(DC->key_state[ALLEGRO_KEY_BACKSPACE] && !DC->prev_key_state[ALLEGRO_KEY_BACKSPACE]) {
				state = STATE::START;
			}
			break;
		}
        case STATE::PAUSE: {
			float slider_x1 = DC->window_width * 0.2f;
			float slider_x2 = DC->window_width * 0.8f;
			float slider_y  = DC->window_height * 0.7f;

			if (DC->mouse_state[1]) {
				int mx = DC->mouse.x;
				int my = DC->mouse.y;
				if (mx >= slider_x1 && mx <= slider_x2 &&
					my >= slider_y - 10 && my <= slider_y + 10) {

					float t = (mx - slider_x1) / (slider_x2 - slider_x1);
					if (t < 0.0f) t = 0.0f;
					if (t > 1.0f) t = 1.0f;
					bgm_volume = t;
					if (bgm_instance) SC->set_volume(bgm_instance, bgm_volume);
				}
			}

			if (DC->key_state[ALLEGRO_KEY_P] && !DC->prev_key_state[ALLEGRO_KEY_P]) {
				debug_log("<Game> state: change to LEVEL\n");
				state = STATE::LEVEL;
			}
			break;
		}

        case STATE::END: {
			if (!game_over_sound_played) {
                SoundCenter* SC = SoundCenter::get_instance();
                if (SC) {
                    SC->play(game_over_sound_path, ALLEGRO_PLAYMODE_ONCE);
                }
                game_over_sound_played = true;
                game_over_timer = 0.0f;
            }

            game_over_timer += static_cast<float>(dt);

            if (DC->key_state[ALLEGRO_KEY_ENTER] && !DC->prev_key_state[ALLEGRO_KEY_ENTER]) {
                return false;
            }
            break;
		}
		case STATE::END_SUCCESS: {
			if (!game_success_sound_played) {
				SoundCenter* SC = SoundCenter::get_instance();
				if (SC) {
					SC->play(game_success_sound_path, ALLEGRO_PLAYMODE_ONCE);
				}
				game_success_sound_played = true;
				game_success_timer = 0.0f;
			}

			game_success_timer += static_cast<float>(dt);

			if (DC->key_state[ALLEGRO_KEY_ENTER] && !DC->prev_key_state[ALLEGRO_KEY_ENTER]) {
				return false;
			}
			break;
		}
    }

    if(state != STATE::PAUSE) {
        DC->player->update();
        SC->update();

        if(state == STATE::LEVEL) {
            OC->update();

            DC->hero->update();

            double hero_cx = DC->hero->shape->center_x();
            double hero_cy = DC->hero->shape->center_y();
            DC->camera_x = hero_cx - DC->window_width  / 2.0f;
            DC->camera_y = hero_cy - DC->window_height / 2.0f;

			if (DC->camera_shake_timer > 0.0f) {
                DC->camera_shake_timer -= (float)dt;
                if (DC->camera_shake_timer < 0.0f)
                    DC->camera_shake_timer = 0.0f;

                float t = DC->camera_shake_timer / 0.4f;
                if (t < 0.0f) t = 0.0f;
                if (t > 1.0f) t = 1.0f;

                float strength = DC->camera_shake_strength * t;

                float offset_x = ((std::rand() / (float)RAND_MAX) * 2.0f - 1.0f) * strength;
                float offset_y = ((std::rand() / (float)RAND_MAX) * 2.0f - 1.0f) * strength;

                DC->camera_x += offset_x;
                DC->camera_y += offset_y;
            }

            DC->starve_info->update(DC->hero->get_starve());
            DC->coin_info->update(DC->hero->get_deposit());
			DC->score_info->update(DC->hero->get_score());

            for(auto b : DC->build) if(b) b->update();
        }

        if(DC->ui && DC->ui->is_open()){
            DC->ui->update();
        }
        if(DC->phone){
            DC->phone->update();
        }
    }

    memcpy(DC->prev_key_state, DC->key_state, sizeof(DC->key_state));
    memcpy(DC->prev_mouse_state, DC->mouse_state, sizeof(DC->mouse_state));
    return true;
}

void
Game::game_draw() {
    DataCenter *DC = DataCenter::get_instance();
    OperationCenter *OC = OperationCenter::get_instance();
    FontCenter *FC = FontCenter::get_instance();

	al_clear_to_color(al_map_rgb(100, 100, 100));

	if (state == STATE::END_SUCCESS) {
		if(background) {
			draw_fullscreen_bitmap(background, DC->window_width, DC->window_height);
		} else {
			al_clear_to_color(al_map_rgb(0,0,0));
		}

		float t = game_success_timer;
		if (t > 1.0f) t = 1.0f;

		unsigned char alpha = (unsigned char)(t * 180);
		al_draw_filled_rectangle(
			0, 0, DC->window_width, DC->window_height,
			al_map_rgba(0, 0, 0, alpha)
		);

		float cx = DC->window_width / 2.f;
		float cy = DC->window_height / 2.f;

		ALLEGRO_COLOR title_col = al_map_rgba(255,255,255,(unsigned char)(255*t));
		al_draw_text(
			FC->caviar_dreams[FontSize::LARGE],
			title_col,
			cx, cy - 60,
			ALLEGRO_ALIGN_CENTRE,
			"YOU WIN!"
		);

		ALLEGRO_COLOR sub_col = al_map_rgba(200,200,200,(unsigned char)(255*t));
		al_draw_text(
			FC->caviar_dreams[FontSize::MEDIUM],
			sub_col,
			cx, cy,
			ALLEGRO_ALIGN_CENTRE,
			"You Successfully Grauated From This Stupid University!"
		);

		int score = DC->hero->get_score();
		std::string tmp = "Final Score: " + std::to_string(score);
		ALLEGRO_COLOR sub_col2 = al_map_rgba(200,200,20,(unsigned char)(255*t));
		al_draw_text(
			FC->caviar_dreams[FontSize::MEDIUM],
			sub_col2,
			cx, cy + 60,
			ALLEGRO_ALIGN_CENTRE,
			tmp.c_str()
		);

		if (t > 0.5f) {
			float hint_t = (t - 0.5f) / 0.5f;
			if (hint_t < 0.0f) hint_t = 0.0f;
			if (hint_t > 1.0f) hint_t = 1.0f;

			ALLEGRO_COLOR hint_col = al_map_rgba(200,200,200,(unsigned char)(255*hint_t));
			al_draw_text(
				FC->caviar_dreams[FontSize::MEDIUM],
				hint_col,
				cx, cy + 140,
				ALLEGRO_ALIGN_CENTRE,
				"Press ENTER to exit"
			);
		}

		al_flip_display();
		return;
	}else if(state == STATE::END) {
		if(background) {
			draw_fullscreen_bitmap(background, DC->window_width, DC->window_height);
		} else {
			al_clear_to_color(al_map_rgb(0, 0, 0));
		}

		if(DC->game_field_length < DC->window_width)
			al_draw_filled_rectangle(
				DC->game_field_length, 0,
				DC->window_width, DC->window_height,
				al_map_rgb(100, 100, 100));

		if(DC->game_field_length < DC->window_height)
			al_draw_filled_rectangle(
				0, DC->game_field_length,
				DC->window_width, DC->window_height,
				al_map_rgb(100, 100, 100));

		ALLEGRO_TRANSFORM camera;
		al_identity_transform(&camera);
		al_translate_transform(&camera, -DC->camera_x, -DC->camera_y);
		al_use_transform(&camera);

		OC->draw();
		DC->map->draw();
		DC->hero->draw();
		DC->level->draw();

		ALLEGRO_TRANSFORM identity;
		al_identity_transform(&identity);
		al_use_transform(&identity);

		if (hero_starved && skull_img) {
			double hx = DC->hero->shape->center_x() - DC->camera_x;
			double hy = DC->hero->shape->center_y() - DC->camera_y;

			float t = game_over_timer;
			if (t > 1.0f) t = 1.0f;

			int sw = al_get_bitmap_width(skull_img);
			int sh = al_get_bitmap_height(skull_img);

			float scale = 1.0f + 0.2f * std::sin(game_over_timer * 3.0f);

			float skull_x = (float)hx;
			float skull_y = (float)hy - 60.0f;

			int smoke_count = 6;
			for (int i = 0; i < smoke_count; ++i) {
				float angle = (float)i / smoke_count * 6.2831853f;
				float base_r = 25.0f;
				float wave   = 6.0f * std::sin(game_over_timer * 2.0f + i);
				float dist   = base_r + wave;

				float sx = (float)hx + std::cos(angle) * dist;
				float sy = (float)hy + std::sin(angle) * dist;

				unsigned char alpha = (unsigned char)(120 * (1.0f - t * 0.5f));
				al_draw_filled_circle(sx, sy, 12.0f, al_map_rgba(0, 0, 0, alpha));
			}

			ALLEGRO_COLOR tint = al_map_rgba_f(1.0f, 1.0f, 1.0f, t);

			al_draw_tinted_scaled_rotated_bitmap(
				skull_img,
				tint,
				sw / 2.0f, sh / 2.0f,
				skull_x, skull_y,
				scale, scale,
				0.0f,
				0
			);
		}

		float fade_t = game_over_timer;
		if (fade_t > 1.0f) fade_t = 1.0f;
		unsigned char alpha = (unsigned char)(fade_t * 200);
		al_draw_filled_rectangle(
			0, 0, DC->window_width, DC->window_height,
			al_map_rgba(0, 0, 0, alpha)
		);

		float cx = DC->window_width  / 2.f;
		float cy = DC->window_height / 2.f;

		float t = game_over_timer;
		if (t > 1.0f) t = 1.0f;

		float title_start_y = cy - 180;
		float title_end_y   = cy - 90;
		float title_y       = title_start_y + (title_end_y - title_start_y) * t;

		ALLEGRO_COLOR title_col = al_map_rgba(
			255, 255, 255,
			(unsigned char)(255 * t)
		);

		al_draw_text(
			FC->caviar_dreams[FontSize::LARGE],
			title_col,
			cx, title_y,
			ALLEGRO_ALIGN_CENTRE,
			"GAME OVER"
		);
		const char* reason_text = hero_starved
			? "You just pass away as like I can't pass my final exam..."
			: "You just pass away as like I can't pass my final exam...";

		ALLEGRO_COLOR reason_col = al_map_rgba(
			230, 200, 200,
			(unsigned char)(255 * t)
		);

		al_draw_text(
			FC->caviar_dreams[FontSize::MEDIUM],
			reason_col,
			cx, title_y + 50,
			ALLEGRO_ALIGN_CENTRE,
			reason_text
		);

		if (t > 0.5f) {
			float hint_t = (t - 0.5f) / 0.5f;
			if (hint_t < 0.0f) hint_t = 0.0f;
			if (hint_t > 1.0f) hint_t = 1.0f;

			ALLEGRO_COLOR hint_col = al_map_rgba(
				200, 200, 200,
				(unsigned char)(255 * hint_t)
			);

			al_draw_text(
				FC->caviar_dreams[FontSize::MEDIUM],
				hint_col,
				cx, cy + 80,
				ALLEGRO_ALIGN_CENTRE,
				"Press ENTER to exit"
			);
		}

		al_flip_display();
		return;
	}

    if(state == STATE::START) {
		if (start_bg) {
			draw_fullscreen_bitmap(start_bg, DC->window_width, DC->window_height);
		} else if (background) {
			draw_fullscreen_bitmap(background, DC->window_width, DC->window_height);
		}

		float cx = DC->window_width / 2.f;

		float btn_w = 260.0f;
		float btn_h = 70.0f;
		float btn_x = cx - btn_w / 2.0f;
		float btn_y = DC->window_height * 0.65f;

		if (start_button) {
			int bw = al_get_bitmap_width(start_button);
			int bh = al_get_bitmap_height(start_button);

			al_draw_scaled_bitmap(
				start_button,
				0, 0, bw, bh,
				btn_x, btn_y,
				btn_w, btn_h,
				0
			);
		} else {
			al_draw_filled_rounded_rectangle(
				btn_x, btn_y,
				btn_x + btn_w, btn_y + btn_h,
				15, 15,
				al_map_rgb(40, 40, 40)
			);
			al_draw_rounded_rectangle(
				btn_x, btn_y,
				btn_x + btn_w, btn_y + btn_h,
				15, 15,
				al_map_rgb(255, 255, 255),
				3
			);
			al_draw_text(
				FC->caviar_dreams[FontSize::MEDIUM], al_map_rgb(255,255,255),
				cx, btn_y + btn_h / 2.0f - 12.0f,
				ALLEGRO_ALIGN_CENTRE, "PLAY");
		}
	}
    else if(state == STATE::HELP) {
        if (help_bg) {
			draw_fullscreen_bitmap(help_bg, DC->window_width, DC->window_height);
		} else if (menu_bg) {
			draw_fullscreen_bitmap(menu_bg, DC->window_width, DC->window_height);
		}

        float cx = DC->window_width / 2.f;
        float y  = DC->window_height / 2.f - 120.f;

        al_draw_text(
            FC->caviar_dreams[FontSize::LARGE], al_map_rgb(255,255,255),
            cx, y,
            ALLEGRO_ALIGN_CENTRE, "GAME INSTRUCTIONS");
        y += 50;

        al_draw_text(
            FC->caviar_dreams[FontSize::MEDIUM], al_map_rgb(255,255,255),
            cx, y,
            ALLEGRO_ALIGN_CENTRE, "Move: W A S D");
        y += 30;

        al_draw_text(
            FC->caviar_dreams[FontSize::MEDIUM], al_map_rgb(255,255,255),
            cx, y,
            ALLEGRO_ALIGN_CENTRE, "Compete with NPCs to snatch food from buildings");
        y += 30;

        al_draw_text(
            FC->caviar_dreams[FontSize::MEDIUM], al_map_rgb(255,255,255),
            cx, y,
            ALLEGRO_ALIGN_CENTRE, "Collect coins, buy items, and avoid starvation");
        y += 40;

        al_draw_text(
            FC->caviar_dreams[FontSize::MEDIUM], al_map_rgb(140,140,140),
            cx, y,
            ALLEGRO_ALIGN_CENTRE, "ENTER → Go to Select Menu");
        y += 25;

        al_draw_text(
            FC->caviar_dreams[FontSize::MEDIUM], al_map_rgb(140,140,140),
            cx, y,
            ALLEGRO_ALIGN_CENTRE, "BACKSPACE → Return to Main Menu");
    }
    else if(state == STATE::UI) {
		if (ui_bg) {
			draw_fullscreen_bitmap(ui_bg, DC->window_width, DC->window_height);
		} else if (select_bg) {
			draw_fullscreen_bitmap(select_bg, DC->window_width, DC->window_height);
		}

		float cx = DC->window_width / 2.f;

		// =========================
		// 1) 固定區塊座標（重點）
		// =========================
		float title_y   = DC->window_height * 0.08f;   // 標題固定
		float preview_y = DC->window_height * 0.16f;   // 英雄預覽固定從這裡開始

		// 設定區(音量/無敵)固定在下半部，不被預覽圖高度影響
		float panel_y   = DC->window_height * 0.60f;   // 想更下面就 0.62/0.65
		float y = panel_y;

		// =========================
		// Title
		// =========================
		al_draw_text(
			FC->caviar_dreams[FontSize::LARGE], al_map_rgb(255,255,255),
			cx, title_y,
			ALLEGRO_ALIGN_CENTRE, "SELECT YOUR HERO"
		);

		// =========================
		// 2) preview animation（保留你原本邏輯）
		// =========================
		static int preview_frame = 0;
		static int preview_counter = 0;

		if (DC->key_state[ALLEGRO_KEY_A] && !DC->prev_key_state[ALLEGRO_KEY_A]) {
			preview_frame = 0;
			preview_counter = HERO_PREVIEW_FREQ;
		}
		if (DC->key_state[ALLEGRO_KEY_D] && !DC->prev_key_state[ALLEGRO_KEY_D]) {
			preview_frame = 0;
			preview_counter = HERO_PREVIEW_FREQ;
		}

		if (preview_counter > 0) {
			--preview_counter;
		} else {
			preview_frame = (preview_frame + 1) % HERO_PREVIEW_FRAME_COUNT;
			preview_counter = HERO_PREVIEW_FREQ;
		}

		// =========================
		// 3) Hero preview（只畫在固定區域，不去動下面 y）
		// =========================
		{
			int idx = selected_hero_index;
			if (idx < 0) idx = 0;
			if (idx >= HERO_TYPE_MAX) idx = HERO_TYPE_MAX - 1;

			const char* base = HERO_NAMES[idx];

			char img_path[256];
			std::snprintf(img_path, sizeof(img_path), "%s/%s/%s_%d.png",
				HERO_PREVIEW_ROOT, base, HERO_DIR_PREFIX, preview_frame);

			ImageCenter* IC = ImageCenter::get_instance();
			ALLEGRO_BITMAP* bmp = IC->get(img_path);

			if (bmp) {
				int bw = al_get_bitmap_width(bmp);
				int bh = al_get_bitmap_height(bmp);

				float max_w = DC->window_width  * 0.35f;
				float max_h = DC->window_height * 0.30f; // ⭐限制預覽最大高度，避免太大
				float scale = 1.0f;
				if (bw > 0 && bh > 0) {
					float sx = max_w / bw;
					float sy = max_h / bh;
					scale = std::min(1.0f, std::min(sx, sy));
				}

				float draw_w = bw * scale;
				float draw_h = bh * scale;

				float hero_x = cx - draw_w / 2.0f;
				float hero_y = preview_y;

				al_draw_scaled_bitmap(
					bmp,
					0, 0, bw, bh,
					hero_x, hero_y,
					draw_w, draw_h,
					0
				);

				float name_y = hero_y + draw_h + 10.0f;
				al_draw_text(
					FC->caviar_dreams[FontSize::MEDIUM], al_map_rgb(255,255,0),
					cx, name_y,
					ALLEGRO_ALIGN_CENTRE, base
				);
			} else {
				al_draw_text(
					FC->caviar_dreams[FontSize::MEDIUM], al_map_rgb(255,120,120),
					cx, preview_y,
					ALLEGRO_ALIGN_CENTRE, "Preview missing"
				);
				al_draw_text(
					FC->caviar_dreams[FontSize::SMALL], al_map_rgb(180,180,180),
					cx, preview_y + 28,
					ALLEGRO_ALIGN_CENTRE, base
				);
			}
		}

		// =========================
		// 4) 操作提示（固定，不吃 preview 高度）
		// =========================
		al_draw_text(
			FC->caviar_dreams[FontSize::MEDIUM], al_map_rgb(140,140,140),
			cx, panel_y - 80,
			ALLEGRO_ALIGN_CENTRE, "A / D : change hero"
		);
		al_draw_text(
			FC->caviar_dreams[FontSize::MEDIUM], al_map_rgb(140,140,140),
			cx, panel_y - 55,
			ALLEGRO_ALIGN_CENTRE, "ENTER : start game"
		);

		// =========================
		// 5) Volume / God Mode（固定在 panel_y 起點）
		// =========================
		float slider_x1 = DC->window_width * 0.2f;
		float slider_x2 = DC->window_width * 0.8f;

		auto draw_slider = [&](float yy, float t, ALLEGRO_COLOR knob_col, float r){
			al_draw_line(slider_x1, yy, slider_x2, yy, al_map_rgb(255,255,255), 3);
			float knob_x = slider_x1 + t * (slider_x2 - slider_x1);
			al_draw_filled_circle(knob_x, yy, r, knob_col);
		};

		// --- Volume ---
		al_draw_text(FC->caviar_dreams[FontSize::MEDIUM], al_map_rgb(255,255,255),
					cx, y, ALLEGRO_ALIGN_CENTRE, "VOLUME");
		y += 28;

		float vol_y = y;
		ui_slider_x1 = slider_x1;
		ui_slider_x2 = slider_x2;
		ui_vol_y = vol_y;

		float vol_t = bgm_volume; if(vol_t < 0) vol_t = 0; if(vol_t > 1) vol_t = 1;
		draw_slider(vol_y, vol_t, al_map_rgb(255,255,0), 10);
		y += 30;

		al_draw_text(FC->caviar_dreams[FontSize::SMALL], al_map_rgb(140,140,140),
					cx, y, ALLEGRO_ALIGN_CENTRE, "Use mouse drag / ← → to adjust volume");
		y += 40;

		// --- Invincible ---
		al_draw_text(FC->caviar_dreams[FontSize::MEDIUM], al_map_rgb(255,255,255),
					cx, y, ALLEGRO_ALIGN_CENTRE, "INVINCIBLE MODE");
		y += 28;

		float god_y = y;
		ui_god_y = god_y;

		float god_t = god_mode ? 1.0f : 0.0f;
		ALLEGRO_COLOR god_col = god_mode ? al_map_rgb(0,160,80) : al_map_rgb(140,140,140);
		draw_slider(god_y, god_t, god_col, 12);
		y += 30;

		al_draw_text(FC->caviar_dreams[FontSize::SMALL], god_col,
					cx, y, ALLEGRO_ALIGN_CENTRE, god_mode ? "ON" : "OFF");
		y += 45;

		al_draw_text(FC->caviar_dreams[FontSize::MEDIUM], al_map_rgb(140,140,140),
					cx, y, ALLEGRO_ALIGN_CENTRE, "BACKSPACE → Return to Main Menu");
	}else {
        if(background) {
            draw_fullscreen_bitmap(background, DC->window_width, DC->window_height);
        }

        if(DC->game_field_length < DC->window_width)
            al_draw_filled_rectangle(
                DC->game_field_length, 0,
                DC->window_width, DC->window_height,
                al_map_rgb(100, 100, 100));

        if(DC->game_field_length < DC->window_height)
            al_draw_filled_rectangle(
                0, DC->game_field_length,
                DC->window_width, DC->window_height,
                al_map_rgb(100, 100, 100));

        ALLEGRO_TRANSFORM camera;
        al_identity_transform(&camera);
        al_translate_transform(&camera, -DC->camera_x, -DC->camera_y);
        al_use_transform(&camera);

        OC->draw();
		DC->level->draw();
        DC->map->draw();
        DC->hero->draw();
		

        ALLEGRO_TRANSFORM identity;
        al_identity_transform(&identity);
        al_use_transform(&identity);

        if(DC->ui && DC->ui->is_open()){
            DC->ui->draw();
        }
        if(DC->phone){
            DC->phone->draw();
        }

		DC->starve_info->draw();
        DC->coin_info->draw();
		DC->time_info->draw();
		DC->score_info->draw();

        if(state == STATE::PAUSE) {
			al_draw_filled_rectangle(
				0, 0, DC->window_width, DC->window_height,
				al_map_rgba(50, 50, 50, 64));

			al_draw_text(
				FC->caviar_dreams[FontSize::LARGE], al_map_rgb(255, 255, 255),
				DC->window_width/2., DC->window_height/2. - 40,
				ALLEGRO_ALIGN_CENTRE, "GAME PAUSED");

			float slider_x1 = DC->window_width * 0.2f;
			float slider_x2 = DC->window_width * 0.8f;
			float slider_y  = DC->window_height * 0.7f;

			al_draw_line(slider_x1, slider_y, slider_x2, slider_y,
						al_map_rgb(255,255,255), 3);

			float t = bgm_volume;
			if (t < 0.0f) t = 0.0f;
			if (t > 1.0f) t = 1.0f;
			float knob_x = slider_x1 + t * (slider_x2 - slider_x1);

			al_draw_filled_circle(knob_x, slider_y, 10, al_map_rgb(255,255,0));

			al_draw_text(
				FC->caviar_dreams[FontSize::SMALL], al_map_rgb(200,200,200),
				DC->window_width / 2.f, slider_y + 20,
				ALLEGRO_ALIGN_CENTRE, "Use mouse drag to adjust volume");

			al_draw_text(
				FC->caviar_dreams[FontSize::MEDIUM], al_map_rgb(200,200,200),
				DC->window_width / 2.f, DC->window_height * 0.85f,
				ALLEGRO_ALIGN_CENTRE, "Press P to resume");
		}
    }

    al_flip_display();
}

Game::~Game() {
	if(display) al_destroy_display(display);
	if(timer) al_destroy_timer(timer);
	if(event_queue) al_destroy_event_queue(event_queue);
}

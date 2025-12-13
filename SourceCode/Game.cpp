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

#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>	
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_acodec.h>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include "data/GIFCenter.h"
#include "algif5/algif.h"

namespace {
    constexpr const char* HERO_PREVIEW_ROOT = "./assets/gif/Hero";
    constexpr const char* HERO_NAMES[] = {
        "dragonite",
        "dog",
        "cat"
    };
    constexpr int HERO_TYPE_MAX = sizeof(HERO_NAMES) / sizeof(HERO_NAMES[0]);
}

constexpr char game_icon_img_path[] = "./assets/image/game_icon.png";
constexpr char game_start_sound_path[] = "./assets/sound/growl.wav";
constexpr char menu_img_path[]           = "./assets/image/MenuBackground.png";
constexpr char select_img_path[]         = "./assets/image/SelectBackground.png";
constexpr char background_img_path[] = "./assets/image/StartBackground.jpg";
constexpr char background_sound_path[] = "./assets/sound/BackgroundMusic.ogg";
constexpr char game_over_sound_path[] = "./assets/sound/game-over-38511.mp3";
constexpr char skull_img_path[] = "./assets/image/skull.png";

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

	menu_bg   = IC->get(menu_img_path);
	select_bg = IC->get(select_img_path);
	background = IC->get(background_img_path);
	skull_img = IC->get(skull_img_path); 

	DC->level->init();
	DC->leveltimer->init();

	bgm_instance = nullptr;
	bgm_volume = 0.4f;

	hero_starved = false;
    game_over_sound_played = false;
    game_over_timer = 0.0f;

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
			DC->leveltimer->update(dt);

			//int timer_level = DC->leveltimer->get_level();

            if(DC->key_state[ALLEGRO_KEY_P] && !DC->prev_key_state[ALLEGRO_KEY_P]) {
				debug_log("<Game> state: change to PAUSE\n");
				state = STATE::PAUSE;
			}
            if(DC->hero->get_starve() <= 0.0) {
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
				if(selected_hero_index < 0)
					selected_hero_index = HERO_TYPE_MAX - 1;
			}
			if(DC->key_state[ALLEGRO_KEY_D] && !DC->prev_key_state[ALLEGRO_KEY_D]) {
				selected_hero_index++;
				if(selected_hero_index >= HERO_TYPE_MAX)
					selected_hero_index = 0;
			}

			float slider_x1 = DC->window_width * 0.2f;
			float slider_x2 = DC->window_width * 0.8f;
			float slider_y  = DC->window_height * 0.7f;

			if(DC->mouse_state[1]) {
				int mx = DC->mouse.x;
				int my = DC->mouse.y;
				if(mx >= slider_x1 && mx <= slider_x2 &&
				my >= slider_y - 10 && my <= slider_y + 10) {

					float t = (mx - slider_x1) / (slider_x2 - slider_x1);
					if(t < 0.0f) t = 0.0f;
					if(t > 1.0f) t = 1.0f;
					bgm_volume = t;
					if(bgm_instance) SC->set_volume(bgm_instance, bgm_volume);
				}
			}
			if(DC->key_state[ALLEGRO_KEY_ENTER] && !DC->prev_key_state[ALLEGRO_KEY_ENTER]) {
				DC->hero->set_type(selected_hero_index);
				DC->hero->init();

				DC->level->load_level(1);
				debug_log("<Game> state: change to LEVEL (GameScene)\n");
				state = STATE::LEVEL;
			}

			if(DC->key_state[ALLEGRO_KEY_BACKSPACE] && !DC->prev_key_state[ALLEGRO_KEY_BACKSPACE]) {
				debug_log("<Game> state: back to START\n");
				state = STATE::START;
			}
			break;
		}
        case STATE::PAUSE: {
			float slider_x1 = DC->window_width * 0.2f;
			float slider_x2 = DC->window_width * 0.8f;
			float slider_y  = DC->window_height * 0.7f;

			if (DC->mouse_state[1]) { // 左鍵按住拖曳
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
			// ★ 進入 END 狀態的第一幀：播 Game Over 音效（只播一次）
			if (!game_over_sound_played) {
                SoundCenter* SC = SoundCenter::get_instance();
                if (SC) {
                    SC->play(game_over_sound_path, ALLEGRO_PLAYMODE_ONCE);
                }
                game_over_sound_played = true;
                game_over_timer = 0.0f;  // 動畫時間從 0 開始
            }

            // 累積 Game Over 經過時間（用來做動畫）
            game_over_timer += static_cast<float>(dt);

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

                // t：0~1，用來讓抖動慢慢減弱
                float t = DC->camera_shake_timer / 0.4f; // 0.4f 要跟上面設定時間一致
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

            for(auto b : DC->build) if(b) b->update();
        }

        if(DC->ui && DC->ui->is_open()){
            DC->ui->update();
        }
        if(DC->phone && DC->phone->is_open()){
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

	if(state == STATE::END) {
		// 1. 先畫遊戲場景（跟 LEVEL 一樣）
		if(background) {
			al_draw_bitmap(background, 0, 0, 0);
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

		// 使用 camera 把世界拉回畫面
		ALLEGRO_TRANSFORM camera;
		al_identity_transform(&camera);
		al_translate_transform(&camera, -DC->camera_x, -DC->camera_y);
		al_use_transform(&camera);

		OC->draw();
		DC->map->draw();
		DC->hero->draw();
		DC->level->draw();

		// 回到畫面座標
		ALLEGRO_TRANSFORM identity;
		al_identity_transform(&identity);
		al_use_transform(&identity);

		// 2. 在主角身上畫「骷髏頭＋黑煙」特效（只有餓死的時候）
		if (hero_starved && skull_img) {
			// 把主角世界座標轉成螢幕座標
			double hx = DC->hero->shape->center_x() - DC->camera_x;
			double hy = DC->hero->shape->center_y() - DC->camera_y;

			float t = game_over_timer;
			if (t > 1.0f) t = 1.0f;

			int sw = al_get_bitmap_width(skull_img);
			int sh = al_get_bitmap_height(skull_img);

			// 讓骷髏頭有一點呼吸縮放（1.0 ~ 1.2 倍）
			float scale = 1.0f + 0.2f * std::sin(game_over_timer * 3.0f);

			// 把骷髏頭放在角色頭上方一點（hy - XX）
			float skull_x = (float)hx;
			float skull_y = (float)hy - 60.0f;   // ★ 調高避免被 GAME OVER 擋到

			// 畫一點黑煙在角色腳邊（簡單幾個半透明圓）
			int smoke_count = 6;
			for (int i = 0; i < smoke_count; ++i) {
				float angle = (float)i / smoke_count * 6.2831853f; // 2π
				float base_r = 25.0f;
				float wave   = 6.0f * std::sin(game_over_timer * 2.0f + i);
				float dist   = base_r + wave;

				float sx = (float)hx + std::cos(angle) * dist;
				float sy = (float)hy + std::sin(angle) * dist;

				unsigned char alpha = (unsigned char)(120 * (1.0f - t * 0.5f));
				al_draw_filled_circle(sx, sy, 12.0f, al_map_rgba(0, 0, 0, alpha));
			}

			// 用 tint 做一點淡入效果（剛 Game Over 時骷髏慢慢變亮）
			ALLEGRO_COLOR tint = al_map_rgba_f(1.0f, 1.0f, 1.0f, t);

			// 以骷髏頭中心為 anchor 來畫
			al_draw_tinted_scaled_rotated_bitmap(
				skull_img,
				tint,
				sw / 2.0f, sh / 2.0f,      // 圖片中心當 pivot
				skull_x, skull_y,          // 螢幕上的位置
				scale, scale,              // 縮放
				0.0f,                      // 不旋轉（要的話可以加晃動）
				0                           // flags
			);
		}

		// 3. 疊一層漸漸變暗的黑幕
		float fade_t = game_over_timer;
		if (fade_t > 1.0f) fade_t = 1.0f;
		unsigned char alpha = (unsigned char)(fade_t * 200); // 最多 200/255 的黑
		al_draw_filled_rectangle(
			0, 0, DC->window_width, DC->window_height,
			al_map_rgba(0, 0, 0, alpha)
		);

		float cx = DC->window_width  / 2.f;
		float cy = DC->window_height / 2.f;

		// 4. GAME OVER 文字從上方滑下來
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

		// 5. 餓死 / 其它原因 額外文字
		const char* reason_text = hero_starved
			? "You starved to death..."
			: "You couldn't survive...";

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

		// 6. 過一小段時間才顯示「Press ENTER to exit」
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
        if(menu_bg) {
            al_draw_bitmap(menu_bg, 0, 0, 0);
        }
        al_draw_text(
            FC->caviar_dreams[FontSize::LARGE], al_map_rgb(255,255,255),
            DC->window_width / 2., DC->window_height / 2.,
            ALLEGRO_ALIGN_CENTRE, "PRESS ENTER TO START");
        al_draw_text(
            FC->caviar_dreams[FontSize::MEDIUM], al_map_rgb(200,200,200),
            DC->window_width / 2., DC->window_height / 2. + 40,
            ALLEGRO_ALIGN_CENTRE, "ESC TO QUIT");
    }
    else if(state == STATE::HELP) {
        if(menu_bg) {
            al_draw_bitmap(menu_bg, 0, 0, 0);
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
            FC->caviar_dreams[FontSize::MEDIUM], al_map_rgb(200,200,200),
            cx, y,
            ALLEGRO_ALIGN_CENTRE, "ENTER → Go to Select Menu");
        y += 25;

        al_draw_text(
            FC->caviar_dreams[FontSize::MEDIUM], al_map_rgb(200,200,200),
            cx, y,
            ALLEGRO_ALIGN_CENTRE, "BACKSPACE → Return to Main Menu");
    }
    else if(state == STATE::UI) {
		if(select_bg) {
			al_draw_bitmap(select_bg, 0, 0, 0);
		}

		float cx = DC->window_width / 2.f;
		float cy = DC->window_height / 2.f - 120.f;

		al_draw_text(
			FC->caviar_dreams[FontSize::LARGE], al_map_rgb(255,255,255),
			cx, cy,
			ALLEGRO_ALIGN_CENTRE, "SELECT YOUR HERO");
		cy += 40;

		{
			int idx = selected_hero_index;
			if(idx < 0) idx = 0;
			if(idx >= HERO_TYPE_MAX) idx = HERO_TYPE_MAX - 1;

			const char* base = HERO_NAMES[idx];

			char gif_path[128];
			sprintf(gif_path, "%s/%s_front.gif", HERO_PREVIEW_ROOT, base);

			GIFCenter* GIFC = GIFCenter::get_instance();
			ALGIF_ANIMATION* gif = GIFC->get(gif_path);

			if(gif) {
				float hero_x = cx - gif->width / 2;
				float hero_y = cy;

				algif_draw_gif(gif, hero_x, hero_y, 0);
				cy += gif->height + 20;

				al_draw_text(
					FC->caviar_dreams[FontSize::MEDIUM], al_map_rgb(255,255,0),
					cx, cy,
					ALLEGRO_ALIGN_CENTRE, base);
				cy += 30;
			}
		}

		al_draw_text(
			FC->caviar_dreams[FontSize::MEDIUM], al_map_rgb(200,200,200),
			cx, cy,
			ALLEGRO_ALIGN_CENTRE, "A / D : change hero");
		cy += 25;

		al_draw_text(
			FC->caviar_dreams[FontSize::MEDIUM], al_map_rgb(200,200,200),
			cx, cy,
			ALLEGRO_ALIGN_CENTRE, "ENTER : start game");
		cy += 25;

		float slider_x1 = DC->window_width * 0.2f;
		float slider_x2 = DC->window_width * 0.8f;
		float slider_y  = DC->window_height * 0.7f;

		al_draw_line(slider_x1, slider_y, slider_x2, slider_y, 
					al_map_rgb(255,255,255), 3);

		float t = bgm_volume;
		if(t < 0.0f) t = 0.0f;
		if(t > 1.0f) t = 1.0f;
		float knob_x = slider_x1 + t * (slider_x2 - slider_x1);

		al_draw_filled_circle(knob_x, slider_y, 10, al_map_rgb(255,255,0));

		al_draw_text(
			FC->caviar_dreams[FontSize::SMALL], al_map_rgb(200,200,200),
			cx, slider_y + 20,
			ALLEGRO_ALIGN_CENTRE, "Use mouse drag / ← → to adjust volume");
		
		al_draw_text(
			FC->caviar_dreams[FontSize::MEDIUM], al_map_rgb(200,200,200),
			cx, DC->window_height * 0.85f,
			ALLEGRO_ALIGN_CENTRE, "BACKSPACE → Return to Main Menu");
	}
    else { 
        if(background) {
            al_draw_bitmap(background, 0, 0, 0);
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

        DC->starve_info->draw();
        DC->coin_info->draw();
		DC->time_info->draw();

        if(DC->ui && DC->ui->is_open()){
            DC->ui->draw();
        }
        if(DC->phone && DC->phone->is_open()){
            DC->phone->draw();
        }

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

			// 白色音量線
			al_draw_line(slider_x1, slider_y, slider_x2, slider_y,
						al_map_rgb(255,255,255), 3);

			// 黃色搖桿位置（依照 bgm_volume）
			float t = bgm_volume;
			if (t < 0.0f) t = 0.0f;
			if (t > 1.0f) t = 1.0f;
			float knob_x = slider_x1 + t * (slider_x2 - slider_x1);

			al_draw_filled_circle(knob_x, slider_y, 10, al_map_rgb(255,255,0));

			// 說明文字
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

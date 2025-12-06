#include "Game.h"
#include "Utils.h"
#include "data/DataCenter.h"
#include "data/OperationCenter.h"
#include "data/SoundCenter.h"
#include "data/ImageCenter.h"
#include "data/FontCenter.h"
#include "Player.h"
#include "Level.h"
#include "object/Build.h"
#include "object/hero.h"
#include "object/ui.h"
#include "object/Phone.h"
#include "info/StarveInfo.h"
#include "info/CoinInfo.h"
#include "Map.h"
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_acodec.h>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <ctime>

// fixed settings
constexpr char game_icon_img_path[] = "./assets/image/game_icon.png";
constexpr char game_start_sound_path[] = "./assets/sound/growl.wav";
constexpr char menu_img_path[]           = "./assets/image/MenuBackground.png";    // 主選單
constexpr char select_img_path[]         = "./assets/image/SelectBackground.png";  // 選角/選關
constexpr char background_img_path[] = "./assets/image/StartBackground.jpg";
constexpr char background_sound_path[] = "./assets/sound/BackgroundMusic.ogg";

/**
 * @brief Game entry.
 * @details The function processes all allegro events and update the event state to a generic data storage (i.e. DataCenter).
 * For timer event, the game_update and game_draw function will be called if and only if the current is timer.
 */
void
Game::execute() {
	DataCenter *DC = DataCenter::get_instance();
	std::srand((unsigned)std::time(nullptr));
	// main game loop
	bool run = true;
	while(run) {
		// process all events here
		al_wait_for_event(event_queue, &event);
		switch(event.type) {
			case ALLEGRO_EVENT_TIMER: {
				run &= game_update();
				game_draw();
				break;
			} case ALLEGRO_EVENT_DISPLAY_CLOSE: { // stop game
				run = false;
				break;
			} case ALLEGRO_EVENT_KEY_DOWN: {
				DC->key_state[event.keyboard.keycode] = true;
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

/**
 * @brief Initialize all allegro addons and the game body.
 * @details Only one timer is created since a game and all its data should be processed synchronously.
 */
Game::Game(bool testMode) {
	DataCenter *DC = DataCenter::get_instance();
	GAME_ASSERT(al_init(), "failed to initialize allegro.");

	// initialize allegro addons
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

	// initialize events
	bool event_init = true;
	event_init &= al_install_keyboard();
	event_init &= al_install_mouse();
	event_init &= al_install_audio();
	GAME_ASSERT(event_init, "failed to initialize allegro events.");

	// initialize game body
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

/**
 * @brief Initialize all auxiliary resources.
 */
void
Game::game_init() {
	DataCenter *DC = DataCenter::get_instance();
	SoundCenter *SC = SoundCenter::get_instance();
	ImageCenter *IC = ImageCenter::get_instance();
	FontCenter *FC = FontCenter::get_instance();
	// set window icon
	game_icon = IC->get(game_icon_img_path);
	al_set_display_icon(display, game_icon);

	// register events to event_queue
    al_register_event_source(event_queue, al_get_display_event_source(display));
    al_register_event_source(event_queue, al_get_keyboard_event_source());
    al_register_event_source(event_queue, al_get_mouse_event_source());
    al_register_event_source(event_queue, al_get_timer_event_source(timer));

	// init sound setting
	SC->init();

	// init font setting
	FC->init();
	DC->ui->init();
	DC->map->init();
	DC->phone->init();
	DC->hero->init();
	DC->starve_info->init();
	DC->coin_info->init();
	
	menu_bg   = IC->get(menu_img_path);
	select_bg = IC->get(select_img_path);
	background = IC->get(background_img_path); // 遊戲中用的背景

	// BGM 初始設定
    bgm_instance = nullptr;
    bgm_volume = 0.4f;  // 預設 40%

	debug_log("Game state: change to START\n");
	state = STATE::START;
	al_start_timer(timer);
}

/**
 * @brief The function processes all data update.
 * @details The behavior of the whole game body is determined by its state.
 * @return Whether the game should keep running (true) or reaches the termination criteria (false).
 * @see Game::STATE
 */
bool
Game::game_update() {
    DataCenter *DC = DataCenter::get_instance();
    OperationCenter *OC = OperationCenter::get_instance();
    SoundCenter *SC = SoundCenter::get_instance();
    static ALLEGRO_SAMPLE_INSTANCE *bgm_instance = nullptr; // BGM 用的 sample instance

    switch(state) {
        // ===== 主選單 =====
        case STATE::START: {
            static bool is_played = false;
            if(!is_played) {
                SC->play(game_start_sound_path, ALLEGRO_PLAYMODE_ONCE);
                is_played = true;
            }

            // 按 ENTER → 進到選角 / 選關畫面
            if(DC->key_state[ALLEGRO_KEY_ENTER] && !DC->prev_key_state[ALLEGRO_KEY_ENTER]) {
                debug_log("<Game> state: change to UI (Select)\n");
                state = STATE::UI;
            }

            // ESC 直接結束遊戲
            if(DC->key_state[ALLEGRO_KEY_ESCAPE] && !DC->prev_key_state[ALLEGRO_KEY_ESCAPE]) {
                return false;
            }
            break;
        }

        // ===== 遊戲主畫面 =====
        case STATE::LEVEL: {
            // 如果前面還沒開始 BGM（例如直接跳 LEVEL），這裡補播
            if(!bgm_instance) {
                bgm_instance = SC->play(background_sound_path, ALLEGRO_PLAYMODE_LOOP, bgm_volume);
            }

            // 用 + / - 微調音量（跟之前邏輯一樣，但改成用 bgm_volume）
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

            // P 暫停
            if(DC->key_state[ALLEGRO_KEY_P] && !DC->prev_key_state[ALLEGRO_KEY_P]) {
                if(bgm_instance) SC->toggle_playing(bgm_instance);
                debug_log("<Game> state: change to PAUSE\n");
                state = STATE::PAUSE;
            }

            // 過關 / 失敗條件
            if(DC->level->remain_monsters() == 0 && DC->monsters.size() == 0) {
                debug_log("<Game> state: change to END\n");
                state = STATE::END;
            }
            if(DC->hero->get_starve() <= 0.0) {
                debug_log("<Game> state: change to END\n");
                state = STATE::END;
            }
            break;
        }

        // ===== 選角 / 選關畫面 =====
        case STATE::UI: {
            // 在選角畫面就開始播 BGM，讓玩家可以試聽音量
            if(!bgm_instance) {
                bgm_instance = SC->play(background_sound_path, ALLEGRO_PLAYMODE_LOOP, bgm_volume);
            }

            // 滑桿位置（要跟 game_draw 裡畫的對齊）
            float slider_x1 = DC->window_width * 0.2f;
            float slider_x2 = DC->window_width * 0.8f;
            float slider_y  = DC->window_height * 0.7f;

            // 滑鼠拖曳調音量（左鍵）
            if(DC->mouse_state[1]) { // 1 = 左鍵
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

            // 也可以用左右鍵微調
            if(DC->key_state[ALLEGRO_KEY_LEFT] && !DC->prev_key_state[ALLEGRO_KEY_LEFT]) {
                bgm_volume -= 0.05f;
                if(bgm_volume < 0.0f) bgm_volume = 0.0f;
                if(bgm_instance) SC->set_volume(bgm_instance, bgm_volume);
            }
            if(DC->key_state[ALLEGRO_KEY_RIGHT] && !DC->prev_key_state[ALLEGRO_KEY_RIGHT]) {
                bgm_volume += 0.05f;
                if(bgm_volume > 1.0f) bgm_volume = 1.0f;
                if(bgm_instance) SC->set_volume(bgm_instance, bgm_volume);
            }

            // ENTER → 開始第 1 關，沿用目前的 bgm_volume
            if(DC->key_state[ALLEGRO_KEY_ENTER] && !DC->prev_key_state[ALLEGRO_KEY_ENTER]) {
                DC->level->load_level(1);
                debug_log("<Game> state: change to LEVEL (GameScene)\n");
                state = STATE::LEVEL;
            }

            // BACKSPACE 回到主選單
            if(DC->key_state[ALLEGRO_KEY_BACKSPACE] && !DC->prev_key_state[ALLEGRO_KEY_BACKSPACE]) {
                debug_log("<Game> state: back to START\n");
                state = STATE::START;
            }
            break;
        }

        // ===== 暫停 =====
        case STATE::PAUSE: {
            if(DC->key_state[ALLEGRO_KEY_P] && !DC->prev_key_state[ALLEGRO_KEY_P]) {
                SC->toggle_playing(bgm_instance);
                debug_log("<Game> state: change to LEVEL\n");
                state = STATE::LEVEL;
            }
            break;
        }

        // ===== 結束 =====
        case STATE::END: {
            return false;
        }
    }

    // ====== 下面是「遊戲內物件的更新」 ======
    if(state != STATE::PAUSE) {
        DC->player->update();
        SC->update();

        // 真正的遊戲邏輯只在 LEVEL 時進行
        if(state == STATE::LEVEL) {
            OC->update();

            DC->hero->update();
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

/**
 * @brief Draw the whole game and objects.
 */
void
Game::game_draw() {
    DataCenter *DC = DataCenter::get_instance();
    OperationCenter *OC = OperationCenter::get_instance();
    FontCenter *FC = FontCenter::get_instance();

    al_clear_to_color(al_map_rgb(100, 100, 100));

    if(state == STATE::END) {
        al_flip_display();
        return;
    }

    if(state == STATE::START) {
        // ===== 主選單 =====
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
    else if(state == STATE::UI) {
		// ===== 選角 / 選關 =====
		if(select_bg) {
			al_draw_bitmap(select_bg, 0, 0, 0);
		}

		al_draw_text(
			FC->caviar_dreams[FontSize::LARGE], al_map_rgb(255,255,255),
			DC->window_width / 2., DC->window_height / 2. - 80,
			ALLEGRO_ALIGN_CENTRE, "SELECT - PRESS ENTER TO PLAY");
		al_draw_text(
			FC->caviar_dreams[FontSize::MEDIUM], al_map_rgb(200,200,200),
			DC->window_width / 2., DC->window_height / 2. - 40,
			ALLEGRO_ALIGN_CENTRE, "BACKSPACE TO MAIN MENU");

		// ===== 音量滑桿 UI =====
		float slider_x1 = DC->window_width * 0.2f;
		float slider_x2 = DC->window_width * 0.8f;
		float slider_y  = DC->window_height * 0.7f;

		// 滑桿底條
		al_draw_filled_rectangle(
			slider_x1, slider_y - 4,
			slider_x2, slider_y + 4,
			al_map_rgb(180, 180, 180));

		// 滑桿圓點位置
		float knob_x = slider_x1 + bgm_volume * (slider_x2 - slider_x1);
		al_draw_filled_circle(
			knob_x, slider_y,
			8.0f,
			al_map_rgb(255, 255, 255));

		// 顯示數字（0~100%）
		int vol_percent = static_cast<int>(bgm_volume * 100.0f + 0.5f);
		char buf[32];
		std::snprintf(buf, sizeof(buf), "VOLUME: %d%%", vol_percent);

		al_draw_text(
			FC->caviar_dreams[FontSize::MEDIUM], al_map_rgb(255,255,255),
			DC->window_width / 2., slider_y + 20,
			ALLEGRO_ALIGN_CENTRE, buf);
	}
    else { // LEVEL 或 PAUSE
        // ===== 遊戲主畫面 =====
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

        // 遊戲物件
        OC->draw();
        DC->map->draw();
        DC->hero->draw();
        DC->starve_info->draw();
        DC->coin_info->draw();

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
                DC->window_width/2., DC->window_height/2.,
                ALLEGRO_ALIGN_CENTRE, "GAME PAUSED");
        }
    }

    al_flip_display();
}

Game::~Game() {
	if(display) al_destroy_display(display);
	if(timer) al_destroy_timer(timer);
	if(event_queue) al_destroy_event_queue(event_queue);
}

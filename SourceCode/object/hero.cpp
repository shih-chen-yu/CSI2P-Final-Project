#include "hero.h"
#include "../data/DataCenter.h"
#include "../data/GIFCenter.h"
#include "../algif5/algif.h"
#include "../shapes/Rectangle.h"
#include "../data/SoundCenter.h"
#include <algorithm>
#include "../object/ui.h"
#include "../object/Bullet.h"

static constexpr const char* coin_up_sfx   = "./assets/sound/coin.mp3";
static constexpr const char* coin_down_sfx = "./assets/sound/cashier.mp3";
namespace HeroSetting{
    static constexpr char gif_root_path[50] = "./assets/gif/Hero";
    static constexpr char gif_postfix[][10] = {
        "left", "right", "front", "back"
    };
    static constexpr char hero_name[][20] = {
        "dragonite",
        "dog",
        "cat"
    };
    static constexpr size_t HERO_TYPE_MAX = sizeof(hero_name) / sizeof(hero_name[0]);
}


void HERO::set_type(int type_index) {
    if (type_index < 0) type_index = 0;
    if (type_index >= (int)HeroSetting::HERO_TYPE_MAX)
        type_index = (int)HeroSetting::HERO_TYPE_MAX - 1;

    hero_type_index = type_index;
}
void HERO::init() {
    using namespace HeroSetting;

    // 根據當前 hero_type_index 決定 base name
    const char* base = hero_name[hero_type_index];

    for (size_t type = 0; type < static_cast<size_t>(HeroState::HEROSTATE_MAX); type++) {
        char buffer[100];
        // "./assets/gif/Hero/<base>_<direction>.gif"
        sprintf(buffer, "%s/%s_%s.gif",
                gif_root_path,
                base,
                gif_postfix[static_cast<int>(type)]);
        gifPath[static_cast<HeroState>(type)] = std::string{buffer};
    }
    DataCenter *DC = DataCenter::get_instance();  
    GIFCenter *GIFC = GIFCenter::get_instance();
    ALGIF_ANIMATION *gif = GIFC->get(gifPath[State]);
    shape.reset(new Rectangle{
        DC->window_width / 2 - gif->width / 2,
        DC->window_height / 2 - gif->height / 2,
        DC->window_width / 2 + gif->width / 2,
        DC->window_height / 2 + gif->height / 2
    });
}


void HERO::draw(){
    GIFCenter* GIFC = GIFCenter::get_instance();
    ALGIF_ANIMATION* gif = GIFC->get(gifPath[State]);
    DataCenter* DC = DataCenter::get_instance();
    if (god_mode) {
        // 角色中心（螢幕座標，因為你 draw() 是世界座標下畫的）
        float cx = shape->center_x();
        float cy = shape->center_y();

        // 呼吸光（閃一下）
        float pulse = 0.5f + 0.5f * std::sin((float)al_get_time() * 6.0f);
        unsigned char a = (unsigned char)(60 + 80 * pulse);

        // 三層圓當 glow
        al_draw_filled_circle(cx, cy + 10, 26, al_map_rgba(80, 180, 255, a));
        al_draw_filled_circle(cx, cy + 10, 18, al_map_rgba(120, 210, 255, a));
        al_draw_filled_circle(cx, cy + 10, 10, al_map_rgba(180, 240, 255, a));
    }
    algif_draw_gif(
        gif, 
        shape->center_x() - gif->width / 2,
        shape->center_y() - gif->height / 2,
        0
    );
}

void HERO::update(){
    DataCenter* DC = DataCenter::get_instance();

    if(starve > 100) starve = 100;
    bool space_pressed = DC->key_state[ALLEGRO_KEY_SPACE] && !DC->prev_key_state[ALLEGRO_KEY_SPACE];
    if (space_pressed) {
        // 取得主角位置與滑鼠位置
        auto [hx, hy] = get_position();
        double mx = DC->mouse.x + DC->camera_x;
        double my = DC->mouse.y + DC->camera_y;

        Bullet* b = new Bullet();
        b->init(hx, hy, mx, my);
        DC->bullets.push_back(b);
        if (!god_mode) {
            starve -= 1.0;
        }
    }

     if(!(DC->ui && DC->ui->is_open())){
        if(DC->key_state[ALLEGRO_KEY_W]){
            shape->update_center_y(shape->center_y() - speed - stamina_extra_speed * (starve / 100.0));
            State = HeroState::BACK;
            if (!god_mode) starve -= starve_decrease_rate_walk;
        }else if(DC->key_state[ALLEGRO_KEY_S]){
            shape->update_center_y(shape->center_y() + speed + stamina_extra_speed * (starve / 100.0));
            State = HeroState::FRONT;
            if (!god_mode) starve -= starve_decrease_rate_walk;
        }else if(DC->key_state[ALLEGRO_KEY_A]){
            shape->update_center_x(shape->center_x() - speed - stamina_extra_speed * (starve / 100.0));
            State = HeroState::LEFT;
            if (!god_mode) starve -= starve_decrease_rate_walk;
        }else if(DC->key_state[ALLEGRO_KEY_D]){
            shape->update_center_x(shape->center_x() + speed + stamina_extra_speed * (starve / 100.0));
            State = HeroState::RIGHT;
            if (!god_mode) starve -= starve_decrease_rate_walk;
        }else{
            // ✅ 站著不動：改回比較慢的 idle 扣法（merge1 的邏輯）
            if (!god_mode) starve -= starve_decrease_rate;
        }
    }
}
void HERO::add_deposit(int reward) {
    if (reward <= 0) return;   // 防呆
    deposit += reward;

    if (auto* SC = SoundCenter::get_instance()) {
        SC->play(coin_up_sfx, ALLEGRO_PLAYMODE_ONCE);
    }
}

void HERO::reduce_deposit(int cost) {
    if (cost <= 0) return;

    int before = deposit;

    deposit -= cost;
    if (deposit < 0) deposit = 0;

    if (deposit != before) {
        if (auto* SC = SoundCenter::get_instance()) {
            SC->play("./assets/sound/cashier.mp3", ALLEGRO_PLAYMODE_ONCE);
        }
    }
}
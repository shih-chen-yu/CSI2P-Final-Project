#include "hero.h"
#include "../data/DataCenter.h"
#include "../data/GIFCenter.h"
#include "../algif5/algif.h"
#include "../shapes/Rectangle.h"
#include "../object/ui.h"


namespace HeroSetting{
    static constexpr char gif_root_path[50] = "./assets/gif/Hero";
    static constexpr char gif_postfix[][10] = {
        "left", "right", "front", "back"
    };
}

void HERO::init(){
    for(size_t type = 0; type < static_cast<size_t>(HeroState::HEROSTATE_MAX); type++){
        char buffer[50];
        sprintf(buffer, "%s/dragonite_%s.gif", HeroSetting::gif_root_path, HeroSetting::gif_postfix[static_cast<int>(type)]);
        gifPath[static_cast<HeroState>(type)] = std::string{buffer};
    }
    GIFCenter *GIFC = GIFCenter::get_instance();
    ALGIF_ANIMATION *gif = GIFC->get(gifPath[State]);
    DataCenter *DC = DataCenter::get_instance();
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

    algif_draw_gif(
        gif, 
        shape->center_x() - gif->width / 2,
        shape->center_y() - gif->height / 2,
        0
    );
}

void HERO::update(){
    DataCenter* DC = DataCenter::get_instance();
    if(!(DC->ui && DC->ui->is_open())){
        if(DC->key_state[ALLEGRO_KEY_W]){
            shape->update_center_y(shape->center_y() - speed);
            State = HeroState::BACK;
            starve -= starve_decrease_rate_walk;
        }else if(DC->key_state[ALLEGRO_KEY_S]){
            shape->update_center_y(shape->center_y() + speed);
            State = HeroState::FRONT;
            starve -= starve_decrease_rate_walk;
        }else if(DC->key_state[ALLEGRO_KEY_A]){
            shape->update_center_x(shape->center_x() - speed);
            State = HeroState::LEFT;
            starve -= starve_decrease_rate_walk;
        }else if(DC->key_state[ALLEGRO_KEY_D]){
            shape->update_center_x(shape->center_x() + speed);
            State = HeroState::RIGHT;
            starve -= starve_decrease_rate_walk;
        }else{
            starve -= starve_decrease_rate;
        }
    }
}
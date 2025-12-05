#include "OperationCenter.h"
#include "DataCenter.h"
#include "../Player.h"

#include "../object/hero.h"
#include "../object/Build.h"
#include "../object/ui.h"

void OperationCenter::update() {
	_update_hero_build();
}

void OperationCenter::_update_hero_build(){
	DataCenter* DC = DataCenter::get_instance();
    HERO* hero = DC->hero;

    // 找出與 hero 重疊的建築（若有多個，選最近的）
    Build* closest = nullptr;
    float best_d2 = DC->window_width * DC->window_width + DC->window_height * DC->window_height;
    for(auto b : DC->build){
        if(!b) continue;
        if(b->shape->overlap(*(hero->shape))){
            float dx = b->shape->center_x() - hero->shape->center_x();
            float dy = b->shape->center_y() - hero->shape->center_y();
            float d2 = dx*dx + dy*dy;
            if(d2 < best_d2){
                best_d2 = d2;
                closest = b;
            }
        }
    }

    // 若沒有任何重疊：全部回 NORMAL，並關閉 UI（若有開啟）
    if(!closest){
        for(auto b : DC->build) if(b) b->change_state(0);
        if(DC->ui && DC->ui->is_open()) DC->ui->close();
        return;
    }

    // 若有焦點 (closest)，把焦點設為 F_NOTIFY，其他都設為 NORMAL
    for(auto b : DC->build){
        if(!b) continue;
        // 若 UI 正在開且目標是這個建築，保留它的狀態 (避免把 F_PRSSED 回覆成 F_NOTIFY)
        if(DC->ui && DC->ui->is_open() && DC->ui->get_target() == b){
            // 不改變狀態，UI 的 target 保持 F_PRSSED
            continue;
        }
        if(b == closest) b->change_state(1);
        else b->change_state(0);
    }

    // 如果 UI 開啟但 target 與 closest 不同 -> 關閉 UI，避免衝突
    if(DC->ui && DC->ui->is_open() && DC->ui->get_target() != closest){
        DC->ui->close();
    }
}

void OperationCenter::draw(){

}

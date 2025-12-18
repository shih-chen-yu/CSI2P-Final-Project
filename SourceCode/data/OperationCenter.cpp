#include "OperationCenter.h"
#include "DataCenter.h"
#include "SoundCenter.h"
#include "../monsters/Monster.h"
#include "../object/hero.h"
#include "../object/Build.h"
#include "../object/ui.h"
#include "../object/Bullet.h"

void OperationCenter::update() {
    _update_monster();
    _update_hero_build();
    _update_bullets();
}

void OperationCenter::_update_monster() {
	std::vector<Monster*> &monsters = DataCenter::get_instance()->monsters;
	for(Monster *monster : monsters)
		monster->update();
}

void OperationCenter::_draw_monster() {
	std::vector<Monster*> &monsters = DataCenter::get_instance()->monsters;
	for(Monster *monster : monsters)
		monster->draw();
}

void OperationCenter::_update_bullets() {
    DataCenter* DC = DataCenter::get_instance();
    SoundCenter* SC = SoundCenter::get_instance();
    auto &bullets = DC->bullets;

    for (auto it = bullets.begin(); it != bullets.end(); ) {
        Bullet* b = *it;
        if (!b) {
            it = bullets.erase(it);
            continue;
        }

        b->update();

        // ===== 子彈撞怪物 =====
        auto &monsters = DC->monsters;
        for (Monster* m : monsters) {
            if (!m) continue;
            if (!m->is_alive()) continue;

            if (b->shape && m->shape && b->shape->overlap(*(m->shape))) {
                DC->hero->add_score(10); // 打中怪物獲得 10 存款
                m->on_hit_by_bullet();
                b->kill();   // 或 b->dead = true; 看你怎麼做

                if (auto* SC = SoundCenter::get_instance()) {
                    SC->play("./assets/sound/fah.mp3", ALLEGRO_PLAYMODE_ONCE);
                }
                break;       // 一顆子彈只打一次
            }
        }

        if (b->is_dead()) {
            delete b;
            it = bullets.erase(it);
        } else {
            ++it;
        }
    }
}

void OperationCenter::_draw_bullets() {
    DataCenter* DC = DataCenter::get_instance();
    auto &bullets = DC->bullets;

    for (Bullet* b : bullets) {
        if (b) b->draw();
    }
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
void OperationCenter::draw() {
	_draw_monster();
    _draw_bullets();
}

#ifndef PHONE_H_INCLUDED
#define PHONE_H_INCLUDED

#include "../Object.h"
#include <string>
#include <vector>
#include <allegro5/allegro.h>

struct FoodInfo {
    std::string building_name;
    std::string message;
    std::string content;

    double create_time = 0.0;
    double life_time   = 8.0;
};

class Phone : public Object{
public:
    void init();
    void update();
    void draw() override;

    bool is_open() { return open; }
    void set_open(bool v) { open = v; }
    void toggle() { open = !open; }

    void add_notification(std::string building_name, std::string message, std::string content) {
        FoodInfo info{building_name, message, content, al_get_time(), 8.0};
        food_infos.push_back(info);
    }

private:
    bool open = false;
    std::vector<FoodInfo> food_infos;

    // ===== 分頁狀態 =====
    int current_page = 0;

    // 用 draw() 算出來後存著（方便 update() clamp）
    int total_pages_cached = 1;
};

#endif

#ifndef PHONE_H_INCLUDED
#define PHONE_H_INCLUDED

#include "../Object.h"
#include <string>
#include <queue>
#include <vector>

#include <allegro5/allegro.h>

struct FoodInfo {
    std::string building_name;
    std::string message;
    std::string content;

    double create_time = 0.0;  // 建立時間（秒）
    double life_time   = 8.0;  // 存活多久（秒），你可以改成你想要的秒數
};

class Phone : public Object{
    public:
        void init();
        void update();
        void draw() override;

        bool is_open() { return open; }
        void set_open(bool v) { open = v; }
        void add_notification(std::string building_name, std::string message, std::string content) {
            FoodInfo info{building_name, message, content, al_get_time(), 8.0};
            food_infos.push_back(info);
        }
    void toggle() { open = !open; }
    private:
        bool open = false;

        std::vector<FoodInfo> food_infos;
};

#endif
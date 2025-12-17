#ifndef PHONE_H_INCLUDED
#define PHONE_H_INCLUDED

#include "../Object.h"
#include <string>
#include <vector>
#include <allegro5/allegro.h>

struct FoodInfo {
    std::string key;
    std::string building_name;
    std::string message;
    std::string content;

    double create_time = 0.0;
    double life_time   = -1.0; // -1 表示不會過期（或你用 1e18）
};

class Phone : public Object{
public:
    void init();
    void update();
    void draw() override;

    bool is_open() { return open; }
    void set_open(bool v) { open = v; }
    void toggle() { open = !open; }

    void add_notification(const std::string& building_name,
                        const std::string& message,
                        const std::string& content)
    {
        // key 用 building_name 當 key（或你想要更獨特也行）
        add_notification(building_name, building_name, message, content);
    }
    void add_notification(const std::string& key,
                          const std::string& building_name,
                          const std::string& message,
                          const std::string& content);

    void upsert_food_status(const std::string& key,
                            const std::string& building_name,
                            const std::string& message,
                            const std::string& content);
    void upsert_food_status(const std::string& building_name,
                            const std::string& message,
                            const std::string& content)
    {
        upsert_food_status(building_name, building_name, message, content);
    }
    void clear_food_status(const std::string& key);

private:
    bool open = false;
    std::vector<FoodInfo> food_infos;

    int current_page = 0;
    int total_pages_cached = 1;

    float icon_angle = 0.0f;
    double spin_end_time = 0.0;
    double spin_duration = 0.4;
    bool spin_active = false;

    float icon_size = 56.0f;
};

#endif

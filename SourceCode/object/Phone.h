#ifndef PHONE_H_INCLUDED
#define PHONE_H_INCLUDED

#include "../Object.h"
#include <string>
#include <vector>
#include <allegro5/allegro.h>

// 一則通知 / 一個狀態列
struct FoodInfo {
    std::string key;            // merge2: 用來 upsert / clear 的唯一 key
    std::string building_name;  // 顯示用
    std::string message;        // 顯示用
    std::string content;        // 顯示用

    double create_time = 0.0;
    double life_time   = -1.0;  // -1: 不會過期（給 upsert_food_status 用）
};

class Phone : public Object {
public:
    void init();
    void update();
    void draw() override;

    bool is_open() const { return open; }
    void set_open(bool v) { open = v; }
    void toggle() { open = !open; }

    // ========== 通知（會過期，預設 30 秒，且一定觸發右下角轉圈） ==========
    void add_notification(const std::string& building_name,
                          const std::string& message,
                          const std::string& content)
    {
        // 3-arg 版：key 預設 = building_name
        add_notification(building_name, building_name, message, content);
    }

    void add_notification(const std::string& key,
                          const std::string& building_name,
                          const std::string& message,
                          const std::string& content);

    // ========== 狀態（不會過期，會被更新/覆寫；用完可 clear） ==========
    void upsert_food_status(const std::string& key,
                            const std::string& building_name,
                            const std::string& message,
                            const std::string& content);

    void upsert_food_status(const std::string& building_name,
                            const std::string& message,
                            const std::string& content)
    {
        // 3-arg 版：key 預設 = building_name
        upsert_food_status(building_name, building_name, message, content);
    }

    void clear_food_status(const std::string& key);

private:
    bool open = false;
    std::vector<FoodInfo> food_infos;

    // ===== 分頁狀態 =====
    int current_page = 0;
    int total_pages_cached = 1; // draw() 算完後回存，update() 會 clamp

    // ===== 右下角小圖示（收到通知必轉圈） =====
    float icon_angle = 0.0f;
    double spin_end_time = 0.0;
    double spin_duration = 0.4;
    bool spin_active = false;

    float icon_size = 56.0f;

    // ===== merge1 小細節：通知預設 30 秒 =====
    static constexpr double kNotificationLife = 30.0;
};

#endif

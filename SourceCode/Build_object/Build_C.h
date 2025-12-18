#ifndef BUILDC_H_INCLUDED
#define BUILDC_H_INCLUDED

#include <string>
#include "../object/Build.h"

class UI;

enum class BuildStateC{
    Nothing,
    Food,
    BuildStateMax
};

class Build_C : public Build{
public:
    void draw_ui(UI* ui, float x, float y, float w, float h) override;
    void update_ui(UI* ui) override;

    BuildStateC get_stateC() const { return StateC; }
    void set_stateC(BuildStateC s);

    // ===== 資源數量（幾份）=====
    int  get_food_amount() const { return food_amount; }
    void set_food_amount(int n);

    // 是否有資源可買/可搶
    bool has_food() const;

    // NPC 盯上狀態（如果你有用）
    bool is_targeted() const { return targeted_by_monster; }
    void set_targeted(bool v) { targeted_by_monster = v; }

    // 有人拿走一份（玩家/NPC）
    bool take_food(int amount = 1, const char* who = "Someone");

protected:
    const char* sprite_basename() const override {
        return "building_C";   // ✅ 記得改成你 C 的圖（別用 building_B）
    }
    void on_interact() override;
    void child_update() override;
    void child_init() override;

private:
    // ===== Phone 同步（跟 A/B 相同套路）=====
    void notify_food_spawn();
    void notify_food_taken(const char* who);
    void sync_phone();

private:
    // ===== 刷新機率（跟 A/B 同一套）=====
    int   frames_passed   = 0;
    int   interval_frames = 60 * 3;

    float base_prob = 0.1f;
    float cur_prob  = 0.1f;
    float prob_step = 0.05f;
    float max_prob  = 0.8f;

    // ===== 狀態/數量 =====
    BuildStateC StateC = BuildStateC::Nothing;

    int  food_amount         = 0;
    bool targeted_by_monster = false;

    // ===== UI 內部確認狀態 =====
    bool in_confirm   = false;
    int  pending_item = 0;  // 1: item1, 2: item2

    std::string ui_message;
    int ui_message_timer = 0;

    // ===== 每棟建築唯一 key（避免多棟覆蓋）=====
    std::string phone_key;
};

#endif


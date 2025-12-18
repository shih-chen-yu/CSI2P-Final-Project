#ifndef BUILDA_H_INCLUDED
#define BUILDA_H_INCLUDED

#include <string>
#include "../object/Build.h"

class UI;

enum class BuildStateA {
    Nothing,
    Food,
    BuildStateMax
};

class Build_A : public Build {
public:
    void draw_ui(UI* ui, float x, float y, float w, float h) override;
    void update_ui(UI* ui) override;

    BuildStateA get_stateA() const { return StateA; }
    void set_stateA(BuildStateA s);          // ✅ 改成宣告（在 .cpp 定義）

    // 食物份數
    int  get_food_amount() const { return food_amount; }
    void set_food_amount(int n);             // ✅ 改成宣告（在 .cpp 定義）

    // 是否還有食物（第二份邏輯）
    bool has_food() const;                   // ✅ 改成宣告（在 .cpp 定義）

    // NPC 搶飯用
    bool is_targeted() const { return targeted_by_monster; }
    void set_targeted(bool v) { targeted_by_monster = v; }

    // ✅ 有人拿走食物（第二份邏輯：會更新 phone）
    bool take_food(int amount = 1, const char* who = "Someone");

protected:
    const char* sprite_basename() const override { return "building_A"; }
    void on_interact() override;
    void child_update() override;
    void child_init() override;

private:
    // ===== phone 狀態同步（第二份邏輯）=====
    void sync_phone();
    void notify_food_spawn();
    void notify_food_taken(const char* who);

private:
    int frames_passed    = 0;
    int interval_frames  = 60 * 3;

    float base_prob = 0.1f;
    float cur_prob  = 0.1f;
    float prob_step = 0.05f;
    float max_prob  = 0.8f;

    BuildStateA StateA = BuildStateA::Nothing;

    // UI 內部「確認購買」狀態
    bool in_confirm   = false;
    int  pending_item = 0;

    // 食物份數 + 是否被盯上
    int  food_amount         = 0;
    bool targeted_by_monster = false;

    // UI 提示
    std::string ui_message;
    int ui_message_timer = 0;

    // ✅ phone 用的 key（每棟不同）
    std::string phone_key;
};

#endif

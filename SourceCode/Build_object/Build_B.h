#ifndef BUILDB_H_INCLUDED
#define BUILDB_H_INCLUDED

#include "../object/Build.h"

class UI;

enum class BuildStateB{
    Nothing,
    Food,
    BuildStateMax
};

class Build_B : public Build{
public:
    void draw_ui(UI* ui, float x, float y, float w, float h) override;
    void update_ui(UI* ui) override;

    BuildStateB get_stateA() const { return StateA; }
    void set_stateA(BuildStateB s) { StateA = s; }

    // ===== 食物相關（之後可以用來做一棟有多份學餐） =====
    int  get_food_amount() const { return food_amount; }
    void set_food_amount(int n) { food_amount = n; }

    // 有沒有食物可以搶：
    // 現在邏輯：state 是 Food 且 food_amount > 0 視為有食物。
    // 如果你暫時沒用 food_amount，也可以只看 StateA。
    bool has_food() const {
        if (food_amount > 0) return true;
        return StateA == BuildStateB::Food;
    }

    // NPC 搶飯用：這棟有沒有被某隻 monster 盯上
    bool is_targeted() const { return targeted_by_monster; }
    void set_targeted(bool v) { targeted_by_monster = v; }

    // 有人（玩家或 NPC）拿走一份
    bool take_food(int amount = 1) {
        if (food_amount > 0) {
            food_amount -= amount;
            if (food_amount <= 0) {
                food_amount = 0;
                StateA = BuildStateB::Nothing;
            }
            return true;
        }

        // 沒有追蹤數量時，單純把 Food 狀態清掉
        if (StateA == BuildStateB::Food) {
            StateA = BuildStateB::Nothing;
            return true;
        }
        return false;
    }

protected:
    const char* sprite_basename() const override {
        return "building_B";  // ./assets/image/building/shop.png
    }
    void on_interact() override;
    void child_update() override;
    void child_init() override;

private:
    int frames_passed    = 0;       // 已經經過幾個 frame
    int interval_frames  = 60 * 3;  // 每 3 秒抽一次（如果 60 FPS）

    float base_prob = 0.1f;   // 初始機率 10%
    float cur_prob  = 0.1f;
    float prob_step = 0.05f;  // 每次沒中，就 +5%
    float max_prob  = 0.8f;   // 機率上限 80%

    BuildStateB StateA = BuildStateB::Nothing;

    // UI 內部「確認購買」的狀態
    bool in_confirm   = false;
    int  pending_item = 0;  // 0: none, 1: drink, 2: bento

    // ===== 新增：食物數量 + 有沒有 NPC 在搶 =====
    int  food_amount         = 0;     // 之後可以真正用來記「有幾份」
    bool targeted_by_monster = false; // 是否有 NPC 正在往這棟衝

    std::string ui_message;
    int ui_message_timer = 0; // frame 倒數（例如 120 = 2 秒）
};

#endif

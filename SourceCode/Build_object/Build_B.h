#ifndef BUILDB_H_INCLUDED
#define BUILDB_H_INCLUDED

#include <string>
#include "../object/Build.h"

class UI;

enum class BuildStateB {
    Nothing,
    Food,
    BuildStateMax
};

class Build_B : public Build {
public:
    void draw_ui(UI* ui, float x, float y, float w, float h) override;
    void update_ui(UI* ui) override;

    BuildStateB get_stateB() const { return StateB; }
    void set_stateB(BuildStateB s);

    // ===== 資源數量（食物/賺錢機會）=====
    int  get_food_amount() const { return food_amount; }
    void set_food_amount(int n);

    // 是否有資源可搶（Food 狀態 且 數量 > 0 或者狀態=Food）
    bool has_food() const;

    // NPC 搶資源用：這棟有沒有被某隻 monster 盯上
    bool is_targeted() const { return targeted_by_monster; }
    void set_targeted(bool v) { targeted_by_monster = v; }

    // 有人（玩家或 NPC）拿走一份
    bool take_food(int amount = 1, const char* who = "Someone");

protected:
    const char* sprite_basename() const override {
        return "building_B";
    }
    void on_interact() override;
    void child_update() override;
    void child_init() override;

private:
    // ===== 手機同步（跟 Build_A 一樣的套路）=====
    void notify_food_spawn();
    void notify_food_taken(const char* who);
    void sync_phone();

private:
    int frames_passed    = 0;
    int interval_frames  = 60 * 3;

    float base_prob = 0.1f;
    float cur_prob  = 0.1f;
    float prob_step = 0.05f;
    float max_prob  = 0.8f;

    BuildStateB StateB = BuildStateB::Nothing;

    bool in_confirm   = false;
    int  pending_item = 0;

    int  food_amount         = 0;
    bool targeted_by_monster = false;

    std::string ui_message;
    int ui_message_timer = 0;

    // 每棟建築唯一 key（避免多棟互相覆蓋）
    std::string phone_key;
};

#endif

#ifndef BUILDA_H_INCLUDED
#define BUILDA_H_INCLUDED

#include <string>
#include "../object/Build.h"

class UI;

enum class BuildStateA{
    Nothing,
    Food,
    BuildStateMax
};

class Build_A : public Build{
public:
    void draw_ui(UI* ui, float x, float y, float w, float h) override;
    void update_ui(UI* ui) override;

    BuildStateA get_stateA() const { return StateA; }
    void set_stateA(BuildStateA s);

    int  get_food_amount() const { return food_amount; }
    void set_food_amount(int n);

    bool has_food() const;
    bool is_targeted() const { return targeted_by_monster; }
    void set_targeted(bool v) { targeted_by_monster = v; }

    bool take_food(int amount = 1, const char* who = "Someone");
protected:
    const char* sprite_basename() const override { return "building_A"; }
    void on_interact() override;
    void child_update() override;
    void child_init() override;

private:
    void sync_phone();              // ✅ 新增：同步手機狀態

private:
    int frames_passed    = 0;
    int interval_frames  = 60 * 3;

    float base_prob = 0.1f;
    float cur_prob  = 0.1f;
    float prob_step = 0.05f;
    float max_prob  = 0.8f;

    BuildStateA StateA = BuildStateA::Nothing;

    bool in_confirm   = false;
    int  pending_item = 0;

    int  food_amount         = 0;
    bool targeted_by_monster = false;

    std::string ui_message;
    int ui_message_timer = 0;
    void notify_food_spawn();
    void notify_food_taken(const char* who);
    std::string phone_key; 
};

#endif

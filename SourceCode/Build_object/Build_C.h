#ifndef BUILDC_H_INCLUDED
#define BUILDC_H_INCLUDED

#include "../object/Build.h"
#include <string>   // ✅ for std::string

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

    bool is_spawned() const { return spawned; }
    void force_spawn(bool v) { spawned = v; }

    void draw() override;   // ⚠️ Build 必須有 virtual draw()

protected:
    const char* sprite_basename() const override {
        return "building_B";
    }
    void on_interact() override;
    void child_update() override;
    void child_init() override;

private:
    // ===== 商店狀態 =====
    BuildStateC StateA = BuildStateC::Nothing;
    bool in_confirm = false;
    int pending_item = 0;

    std::string ui_message;
    int ui_message_timer = 0;

    // ===== 出現/消失參數 =====
    bool spawned = false;
    int  spawn_check_frames = 60 * 3;
    int  spawn_check_cnt = 0;

    float spawn_prob = 0.10f;
    int  stay_frames = 60 * 12;
    int  stay_cnt = 0;

    // ===== 魔爪刷貨 =====
    bool claw_available = false;
    float claw_prob = 0.45f;
};

#endif

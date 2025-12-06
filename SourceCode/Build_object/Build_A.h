#ifndef BUILDA_H_INCLUDED
#define BUILDA_H_INCLUDED

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
    protected:
        const char* sprite_basename() const override {
            return "building2";  // ./assets/image/building/shop.png
        }
        void on_interact() override;
        void child_update() override;
        void child_init() override;
    private:
        int frames_passed = 0;    // 已經經過幾個 frame
        int interval_frames = 60 * 3; // 每 5 秒抽一次（如果 60 FPS）

        float base_prob = 0.1f;   // 初始機率 10%
        float cur_prob = 0.1f;
        float prob_step = 0.05f;  // 每次沒中，就 +5%
        float max_prob  = 0.8f;   // 機率上限 80%

        BuildStateA StateA = BuildStateA::Nothing;

        // UI 內部「確認購買」的狀態
        bool in_confirm   = false;
        int  pending_item = 0;  // 0: none, 1: drink, 2: bento
};

#endif
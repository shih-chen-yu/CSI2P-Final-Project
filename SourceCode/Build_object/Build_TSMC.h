#ifndef BUILD_TSMC_H_INCLUDED
#define BUILD_TSMC_H_INCLUDED

#include "../object/Build.h"
#include <string>

class UI;

class Build_TSMC : public Build {
public:
    void draw_ui(UI* ui, float x, float y, float w, float h) override;
    void update_ui(UI* ui) override;

protected:
    const char* sprite_basename() const override {
        // 對應 ./assets/image/building/building_TSMC.png
        return "building_TSMC";
    }

    void on_interact() override;
    void child_update() override;
    void child_init() override;

private:
    // ---- 選單行為種類 ----
    enum class TSMCChoice {
        None,
        AMBAFood,   // AMBA 剩食便當
        RobBoss,    // 打劫大老闆
        Buy711,     // 7-11 買食物
    };

    // UI 狀態
    bool in_confirm = false;   // 是否在確認畫面
    TSMCChoice pending_choice = TSMCChoice::None;

    std::string result_message;          // 下方結果訊息
    int result_timer = 0;      // 結果訊息剩餘 frame

    // 事件可用狀態
    bool amba_food_available = false;   // 是否有 AMBA 剩食
    bool boss_available = false;   // 是否目前有大老闆在上課
    bool store_food_available = false;   // 7-11 是否有食物

    // 時間驅動
    int frames_passed = 0;             // 已累積 frame
    int interval_frames = 60 * 5;        // 每 5 秒檢查一次事件（以 60FPS 為例）
};

#endif
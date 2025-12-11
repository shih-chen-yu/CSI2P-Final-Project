#ifndef BUILD_HUMAN_H_INCLUDED
#define BUILD_HUMAN_H_INCLUDED

#include "../object/Build.h"
#include <string>

class UI;

// 人社院裡可以選擇的行為
enum class HumanChoice {
    None,
    Lecture,  // 領取講座便當（免費）
    Mcdonald  // 購買麥當勞（花錢，有冷卻）
};

class Build_human : public Build {
public:
    void draw_ui(UI* ui, float x, float y, float w, float h) override;
    void update_ui(UI* ui) override;

protected:
    const char* sprite_basename() const override {
        // 對應 ./assets/image/building/building_human.png
        return "building_human";
    }

    void on_interact() override;
    void child_update() override;
    void child_init() override;

private:
    // UI 流程
    bool in_confirm = false;  // 是否在確認畫面
    HumanChoice pending_choice = HumanChoice::None;

    // 顯示在 UI 下方的一小段結果文字（例如「飽食度 +40」）
    std::string result_message;
    int result_timer   = 0;      // 還要顯示幾個 frame

    // 講座免費午餐是否目前可領
    bool lecture_available = false;
    int frames_passed = 0;      // 用來判定何時檢查新講座事件
    int interval_frames = 60 * 5; // 每 5 秒檢查一次（60FPS）

    // 麥當勞冷卻時間：在冷卻內，人社院功能無法使用
    int mcd_cooldown_frames = 0;      // >0 表示冷卻中
};

#endif

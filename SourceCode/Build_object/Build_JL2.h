#ifndef BUILD_JL2_H_INCLUDED
#define BUILD_JL2_H_INCLUDED

#include "../object/Build.h"
#include <string>

class UI;

// 玩家在 UI 裡可以選擇的行為（JL2 專用）
enum class JL2Choice {
    None,
    Exam,
    ProfessorTask
};

class Build_JL2 : public Build {
public:
    void draw_ui(UI* ui, float x, float y, float w, float h) override;
    void update_ui(UI* ui) override;

protected:
    const char* sprite_basename() const override {
        return "building_edu";
    }

    void on_interact() override;
    void child_update() override;
    void child_init() override;

private:
    // ===== UI 狀態 =====
    bool        in_confirm     = false;              // 是否在確認畫面
    JL2Choice   pending_choice = JL2Choice::None;

    std::string result_message;                      // 底部提示文字
    int         result_timer   = 0;                  // 剩餘顯示 frame 數

    // ===== 隨機事件控制 =====
    int  frames_passed   = 0;                        // 每隔一段時間 roll
    int  interval_frames = 60 * 5;                   // 約每 5 秒（60FPS）

    // ----- 突發考試相關 -----
    bool exam_active          = false;               // 是否有倒數中的考試
    int  exam_remain_frames   = 0;                   // 剩餘時間（frame）
    bool exam_penalty_pending = false;               // 是否待結算扣錢

    // ----- 教授任務相關 -----
    bool professor_task_available = false;            // 是否有任務可接
    int  professor_task_cooldown  = 0;                // 冷卻時間（frame）
};

#endif // BUILD_JL2_H_INCLUDED

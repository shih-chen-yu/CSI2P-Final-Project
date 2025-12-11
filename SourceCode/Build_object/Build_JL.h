#ifndef BUILD_GL_H_INCLUDED
#define BUILD_GL_H_INCLUDED

#include "../object/Build.h"
#include <string>

class UI;

// 玩家在 UI 裡可以選擇的行為
enum class JLChoice {
    None,
    ProfessorTask
};

class Build_JL : public Build {
public:
    void draw_ui(UI* ui, float x, float y, float w, float h) override;
    void update_ui(UI* ui) override;

protected:
    const char* sprite_basename() const override {
        // 對應 ./assets/image/building/building_JL.png
        return "building_JL";
    }

    void on_interact() override;
    void child_update() override;
    void child_init() override;

private:
    // ===== UI 狀態 =====
    bool        in_confirm     = false;   // 是否在確認畫面
    JLChoice    pending_choice = JLChoice::None;

    std::string result_message;           // 底部提示文字（例如：+50 元）
    int         result_timer   = 0;       // 剩餘顯示 frame 數

    // ===== 隨機事件控制 =====
    int  frames_passed   = 0;            // 用來每隔一段時間 roll 一次事件
    int  interval_frames = 60 * 5;       // 約每 5 秒 roll 一次（60FPS 假設）

    // ----- 突發考試相關 -----
    bool exam_active          = false;   // 是否有一個正在倒數的突發考試
    int  exam_remain_frames   = 0;       // 考試剩餘時間（frame）
    bool exam_penalty_pending = false;   // 是否尚未結算的「缺考扣錢」懲罰

    // ----- 教授任務相關 -----
    bool professor_task_available = false; // 是否目前有任務可以接
    int  professor_task_cooldown  = 0;     // 任務完成後的冷卻（frame）
};

#endif

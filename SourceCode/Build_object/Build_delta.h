#ifndef BUILD_DELTA_H_INCLUDED
#define BUILD_DELTA_H_INCLUDED

#include "../object/Build.h"
#include <string>

class UI;
class DataCenter;
enum class DeltaFoodKind {
    None,
    Office,  // 系辦剩食
    Camp     // 營隊剩食
};

class Build_delta : public Build {
public:
    void draw_ui(UI* ui, float x, float y, float w, float h) override;
    void update_ui(UI* ui) override;

protected:
    const char* sprite_basename() const override {
        // ./assets/image/building/building_delta.png 之類
        return "building_delta";
    }

    void on_interact() override;
    void child_update() override;
    void child_init() override;
    

private:
    // 每隔一段時間檢查一次兩種活動
    int frames_passed   = 0;
    int interval_frames = 60 * 5;  // 假設 60 FPS → 每 5 秒檢查一次

    // 兩種活動是否目前「有剩食可領」
    int office_count = 0; // 系辦剩食剩餘份數
    int camp_count = 0;   // 營隊剩食剩餘份數

    // UI 二階段確認用
    bool          in_confirm   = false;
    DeltaFoodKind pending_kind = DeltaFoodKind::None;
    std::string result_message;  
    int result_timer = 0;        // 顯示多久（frame 或秒）
    void sync_phone_status(DataCenter* DC);
};

#endif
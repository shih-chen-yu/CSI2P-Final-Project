#ifndef INFO_H_INCLUDED
#define INFO_H_INCLUDED

class StarveInfo {
public:
    StarveInfo();
    void init();
    void update(int data);
    void draw();
    void trigger_poison_flash(); 
private:
    float x,y,w,h; // bar rect
    float padding;
    float progress; // 0..1
    float target_progress;  // 目標值 0..1（對應 hero 的真實飢餓）
    float anim_speed;       // 每秒追蹤速度
    float highlight_timer;  // 高亮顏色剩餘時間（秒）
    bool initialized;
    float poison_timer;
};

#endif
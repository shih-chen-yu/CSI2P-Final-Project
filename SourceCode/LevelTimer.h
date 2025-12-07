// LevelTimer.h
#ifndef LEVEL_TIMER_H_INCLUDED
#define LEVEL_TIMER_H_INCLUDED

class LevelTimer {
public:
    LevelTimer();  // constructor 不做參數設定，只做安全初始化

    // 用來初始化這個 timer
    void init(double interval_sec = 30.0, int start_level = 1);

    // 每一 frame 呼叫一次，dt = 這一 frame 過了幾秒
    void update(double dt);

    int  get_level() const;
    void reset(int new_level = 1);
    void set_interval(double sec);

private:
    double interval_sec;  // 每隔幾秒升級一次
    double elapsed_sec;   // 自從上次升級以來累積了幾秒
    int    level;         // 目前等級
};

#endif

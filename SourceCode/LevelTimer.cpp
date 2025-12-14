// LevelTimer.cpp
#include "LevelTimer.h"
#include "Utils.h"
#include "info/TimeInfo.h"
#include "data/DataCenter.h"

LevelTimer::LevelTimer()
    : interval_sec(60.0), elapsed_sec(0.0), level(1) {
    // 這裡只做「安全的初始值」
    // 真正的設定由 init() 負責
}

void LevelTimer::init(double interval_sec, int start_level) {
    this->interval_sec = interval_sec;
    this->elapsed_sec  = 0.0;
    this->level        = start_level;
}

void LevelTimer::update(double dt) {
    // 如果 interval_sec 沒設定（=0），就當作關閉
    if (interval_sec <= 0.0) return;

    DataCenter* DC = DataCenter::get_instance();
    elapsed_sec += dt;

    while (elapsed_sec >= interval_sec) {
        elapsed_sec -= interval_sec;
        ++level;
        debug_log("level up\n");
    }
    DC->time_info->update(level);
}

int LevelTimer::get_level() const {
    return level;
}

void LevelTimer::reset(int new_level) {
    level = new_level;
    elapsed_sec = 0.0;
}

void LevelTimer::set_interval(double sec) {
    interval_sec = sec;
}

#ifndef TIMEINFO_H_INCLUDED
#define TIMEINFO_H_INCLUDED

class TimeInfo {
public:
    TimeInfo();
    void init();
    void update(int data);
    void draw();
private:
    float x,y,w,h; // bar rect
    float padding;
    int level;
};

#endif
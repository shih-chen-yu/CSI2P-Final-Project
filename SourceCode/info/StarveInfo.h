#ifndef INFO_H_INCLUDED
#define INFO_H_INCLUDED

class StarveInfo {
public:
    StarveInfo();
    void init();
    void update(int data);
    void draw();
private:
    float x,y,w,h; // bar rect
    float padding;
    float progress; // 0..1
};

#endif
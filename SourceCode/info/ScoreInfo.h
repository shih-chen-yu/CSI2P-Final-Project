#ifndef SCOREINFO_H_INCLUDED
#define SCOREINFO_H_INCLUDED

class ScoreInfo {
public:
    ScoreInfo();
    void init();
    void update(int data);
    void draw();

private:
    float x, y, w, h;   // bar rect
    float padding;
    int score;
};

#endif
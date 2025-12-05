#ifndef UI_H_INCLUDED
#define UI_H_INCLUDED

#include <allegro5/allegro_primitives.h>

class Build;

class UI {
public:
    UI();
    void init();
    void open(Build* target);
    void close();
    void update();
    void draw();
    bool is_open() const;
    Build* get_target() const;
private:
    bool open_flag;
    Build* target_build;
    float x,y,w,h; // panel rect
};

#endif
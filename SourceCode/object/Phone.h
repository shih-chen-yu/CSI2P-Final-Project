#ifndef PHONE_H_INCLUDED
#define PHONE_H_INCLUDED

#include "../Object.h"

class Phone : public Object{
    public:
        void init();
        void update();
        void draw() override;
        bool is_open() { return open; }
    private:
        bool open = false;
};

#endif
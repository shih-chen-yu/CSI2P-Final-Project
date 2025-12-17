#ifndef COININFO_H_INCLUDED
#define COININFO_H_INCLUDED

class CoinInfo {
public:
    CoinInfo();
    void init();
    void update(int data);
    void draw();
private:
    float x,y,w,h; // bar rect
    float padding;
    int coin;
    int prev_coin = 0;
    bool first_update = true;
};

#endif
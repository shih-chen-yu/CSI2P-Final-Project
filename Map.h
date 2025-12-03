#ifndef MAP_H_INCLUDED
#define MAP_H_INCLUDED

#include <vector>

#include "data/DataCenter.h"
#include "object/Build.h"

class Map{
    public:
        void init(){
            // 初始化地圖資料 (比例座標; 0..1)
            map_data = {
                {0.2f, 0.5f},
                {0.5f, 0.5f},
                {0.8f, 0.3f},
            };
            // 不在這裡直接 draw(); 讓 game loop 呼叫 draw()
        }
        void draw(){
            DataCenter* DC = DataCenter::get_instance();
            // 對每個比例座標：建立或放置 Build，然後 draw
            for(size_t i=0;i<map_data.size();++i){
                float rx = map_data[i].first;
                float ry = map_data[i].second;
                // 轉成遊戲區域實際座標 (使用 game_field_length)
                float abs_x = rx * DC->game_field_length;
                float abs_y = ry * DC->game_field_length;
                // 若 DC->build 尚未有這個 index，建立一個新的 Build
                if(DC->build.size() <= i){
                    Build* b = new Build();
                    b->init();
                    b->set_center(abs_x, abs_y);
                    DC->build.push_back(b);
                } else {
                    DC->build[i]->set_center(abs_x, abs_y);
                }
                // 將該建築畫出
                if(DC->build[i]) DC->build[i]->draw();
            }
        }
    private:
        std::vector<std::pair<float,float>> map_data;
};

#endif
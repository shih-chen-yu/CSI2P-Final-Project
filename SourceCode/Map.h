#ifndef MAP_H_INCLUDED
#define MAP_H_INCLUDED

#include <vector>

#include "data/DataCenter.h"
#include "object/Build.h"

#include "Build_object/Build_A.h"

enum class BuildKind {
    Normal,   // 原本的建築
    Normal2
};

struct MapBuildInfo {
    float rx;
    float ry;
    BuildKind kind;
};

class Map{
    public:
        void init(){
            // 初始化地圖資料 (比例座標; 0..1)
            map_data = {
                {0.2f, 0.5f, BuildKind::Normal},     // 左邊這棟是商店
                {0.5f, 0.5f, BuildKind::Normal},  // 中間這棟是圖書館
                {0.8f, 0.3f, BuildKind::Normal2},     // 右邊這棟是宿舍
            };
            // 不在這裡直接 draw(); 讓 game loop 呼叫 draw()
        }
        void draw(){
            DataCenter* DC = DataCenter::get_instance();
            // 對每個比例座標：建立或放置 Build，然後 draw
            for(size_t i=0;i<map_data.size();++i){
                float rx = map_data[i].rx;
                float ry = map_data[i].ry;
                
                // 轉成遊戲區域實際座標 (使用 game_field_length)
                float abs_x = rx * DC->game_field_length;
                float abs_y = ry * DC->game_field_length;

                // 如果這個 index 還沒有建築，就依照 kind 建一個對應的子類別
                if(DC->build.size() <= i){
                    Build* b = create_build_of_kind(map_data[i].kind); // ★ 用類型決定 new 哪個 class
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
        Build* create_build_of_kind(BuildKind kind) {
            switch(kind){
                case BuildKind::Normal2:
                    return new Build_A();
                case BuildKind::Normal:
                default:
                    return new Build();
            }
        }
        std::vector<MapBuildInfo> map_data;
};

#endif
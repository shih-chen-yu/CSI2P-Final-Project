#ifndef MAP_H_INCLUDED
#define MAP_H_INCLUDED

#include <vector>

#include "data/DataCenter.h"
#include "object/Build.h"

#include "Build_object/Build_A.h"
#include "Build_object/Build_delta.h"
#include "Build_object/Build_human.h"
#include "Build_object/Build_TSMC.h"
#include "Build_object/Build_JL.h"

enum class BuildKind {
    Normal,   // 原本的建築
    Normal2,
    DELTA, //台達館
    HUMAN, //人社院
    TSMC,  //台積館
    JL     //炯朗館
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
                {0.0f, 0.5f, BuildKind::TSMC},
                {0.5f, 0.0f, BuildKind::DELTA},
                {0.8f, 0.3f, BuildKind::Normal2},
                {1.1f, 0.7f, BuildKind::HUMAN},
                {0.3f, 0.0f, BuildKind::JL},
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
                case BuildKind::DELTA:
                    return new Build_delta();
                case BuildKind::HUMAN:
                    return new Build_human();
                case BuildKind::TSMC:
                    return new Build_TSMC();
                case BuildKind::JL:
                    return new Build_JL();
                case BuildKind::Normal:
                default:
                    return new Build();
            }
        }
        std::vector<MapBuildInfo> map_data;
};

#endif
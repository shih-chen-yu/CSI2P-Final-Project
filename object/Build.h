#ifndef BUILD_H_INCLUDED
#define BUILD_H_INCLUDED

#include "../Object.h"
#include<string>
#include<map>

class UI;

enum class BuildState{
    NORMAL,
    F_NOTIFY,
    F_PRSSED,
    BUILDSTATE_MAX
};

class Build : public Object{
    public:
        void init();
        void update();
        void draw() override;

        /**
         * @brief 修改建築圖案(0=>正常, 1=>顯示F的圖案)
         * @details 0=>正常, 1=>顯示F的圖案
         */
        void change_state(int state);
        void set_center(float cx, float cy); // 新增：設定建築中心位置
        
        virtual void draw_ui(UI* ui, float x, float y, float w, float h);
        virtual void update_ui(UI* ui);
    protected:
        virtual const char* sprite_basename() const { return "building"; }
        virtual void on_interact();
        virtual void child_update();

        BuildState State = BuildState::NORMAL;
        std::map<BuildState, std::string> picpath;
        std::string main_picpath;
        std::string hint_picpath;
        float scale;
        float hint_scale;
};

#endif
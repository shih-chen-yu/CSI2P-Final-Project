#include "../object/Build.h"

class Build_A : public Build{
    public:
        void draw_ui(UI* ui, float x, float y, float w, float h);
        void update_ui(UI* ui);
    protected:
        const char* sprite_basename() const override {
            return "building2";  // ./assets/image/building/shop.png
        }
        void on_interact();
};
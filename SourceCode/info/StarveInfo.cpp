#include "../data/DataCenter.h"
#include "../data/FontCenter.h"
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <algorithm>
#include "StarveInfo.h"
#include "../object/hero.h"
#include "../data/SoundCenter.h" 
#include <cmath>              
#include <cstdio> 

StarveInfo::StarveInfo()
    : x(0), y(0), w(150), h(18),
      padding(12),
      progress(0.0f),
      target_progress(0.0f),
      anim_speed(2.5f),       // 每秒最多變 2.5（約 0.4 秒追上）
      highlight_timer(0.0f),   // 一開始沒有高亮
      initialized(false),
      poison_timer(0.0f) 
{}

void StarveInfo::init(){
    DataCenter* DC = DataCenter::get_instance();
    // 右上角，留一點 padding
    x = DC->window_width - padding - w;
    y = padding;
}

void StarveInfo::update(int data){
    DataCenter* DC = DataCenter::get_instance();

    // 1. 轉成 0..1 的目標值
    double s = static_cast<double>(data);
    double t = s / 100.0;
    if(t < 0.0) t = 0.0;
    if(t > 1.0) t = 1.0;
    float new_target = static_cast<float>(t);

    if (poison_timer > 0.0f) {
        float dt = 1.0f / (float)DC->FPS;
        poison_timer -= dt;
        if (poison_timer < 0.0f)
            poison_timer = 0.0f;
    }

    // 2. 如果這一幀「目標值比之前高」，代表吃到食物 → 高亮 + 音效
    if (!initialized) {
        target_progress = new_target;
        progress = new_target;
        initialized = true;
        return;
    }

    if (new_target > target_progress + 1e-4f) {
        highlight_timer = 0.3f;

        SoundCenter* SC = SoundCenter::get_instance();
        if (SC) {
            SC->play("./assets/sound/eat.wav", ALLEGRO_PLAYMODE_ONCE);
        }
    }

    // 更新目標值
    target_progress = new_target;

    // 3. 依 FPS 算出 dt（每幀時間）
    double dt = 1.0 / DC->FPS;
    float step = anim_speed * static_cast<float>(dt);

    // 讓 progress 慢慢逼近 target_progress
    float diff = target_progress - progress;
    if (std::fabs(diff) <= step) {
        progress = target_progress;
    } else {
        progress += (diff > 0.0f ? step : -step);
    }

    // 4. 更新高亮計時器
    if (highlight_timer > 0.0f) {
        highlight_timer -= static_cast<float>(dt);
        if (highlight_timer < 0.0f) highlight_timer = 0.0f;
    }
}

void StarveInfo::draw(){
    FontCenter* FC = FontCenter::get_instance();

    // 背景條（黑色半透明外框）
    al_draw_filled_rectangle(x - 2, y - 2, x + w + 2, y + h + 2, al_map_rgba(0,0,0,160));
    // 底色（灰色）
    al_draw_filled_rectangle(x, y, x + w, y + h, al_map_rgba(80,80,80,200));

    // 基本顏色：依 progress 在 green -> yellow -> red 之間變化
    float r = progress < 0.5f ? (1.0f - progress*2.0f) : 0.0f;
    float g = progress < 0.5f ? (progress*2.0f) : (1.0f - (progress-0.5f)*2.0f);
    float b = progress > 0.5f ? (progress-0.5f)*2.0f : 0.0f;

    // 如果剛吃到東西（highlight_timer > 0），顏色變更亮、更偏黃一點
    if (highlight_timer > 0.0f) {
        float total_highlight = 0.3f;      // 要跟 update 裡設定的一樣
        float t = highlight_timer / total_highlight;
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;

        // 往「亮黃白」(1,1,0.5) 混
        r = r * (1.0f - t) + 1.0f * t;
        g = g * (1.0f - t) + 1.0f * t;
        b = b * (1.0f - t) + 0.5f * t;
    }
    if (poison_timer > 0.0f) {
        float total = 0.35f;                    // 要跟 trigger_poison_flash 一樣
        float t = poison_timer / total;         // 1 → 0
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;

        // 基礎毒色：偏綠紫（你可以調）
        float base_r = 0.5f;
        float base_g = 1.0f;
        float base_b = 0.2f;

        // 讓顏色亮度閃一下（用 sin 搖）
        float pulse = 0.3f * std::sin(poison_timer * 20.0f);
        base_r += pulse;
        base_g += pulse;
        if (base_r > 1.0f) base_r = 1.0f;
        if (base_g > 1.0f) base_g = 1.0f;

        // 也可以混一點紫
        base_b = 0.8f;

        r = base_r;
        g = base_g;
        b = base_b;
    }

    ALLEGRO_COLOR col = al_map_rgba_f(r, g, b, 1.0f);

    // 實際的進度條
    al_draw_filled_rectangle(x, y, x + w * progress, y + h, col);

    // 外框
    al_draw_rectangle(x, y, x + w, y + h, al_map_rgb(255,255,255), 2.0f);

    // 文字顯示百分比
    if(FC){
        char buf[32];
        int pct = static_cast<int>(progress * 100.0f + 0.5f);
        std::sprintf(buf, "Starve: %d%%", pct);
        al_draw_text(
            FC->caviar_dreams[FontSize::SMALL],
            al_map_rgb(255,255,255),
            x + w / 2.0f, y + h + 6.0f,
            ALLEGRO_ALIGN_CENTRE, buf
        );
    }
}
void StarveInfo::trigger_poison_flash() {
    poison_timer = 0.35f;   // 中毒顏色維持大約 0.35 秒
}
#include "Phone.h"
#include "../data/DataCenter.h"
#include "../data/FontCenter.h"
#include "../Utils.h"

#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>

// ================= helper: 依照最大寬度把文字切成多行 =================
// 這版用「逐字累加」最穩（中英都能 work，只是中文會以字為單位）
static std::vector<std::string> wrap_text(ALLEGRO_FONT* font, const std::string& s, float max_w) {
    std::vector<std::string> lines;
    std::string cur;

    auto flush = [&]() {
        if (!cur.empty()) lines.push_back(cur);
        cur.clear();
    };

    for (size_t i = 0; i < s.size(); ) {
        // 取一個 UTF-8 字元（簡化版：先當作 byte-by-byte；如果你內容主要是中文，這裡仍可用，但會較粗）
        // 若你確定通知文字都是 ASCII/英文，可直接 char c = s[i++]
        // 若你要正確 UTF-8 分割，之後我可以再帶你改成真正的 utf8 split。

        char c = s[i++];
        std::string add(1, c);

        // 遇到 '\n' 強制換行
        if (c == '\n') {
            flush();
            continue;
        }

        std::string next = cur + add;
        if (al_get_text_width(font, next.c_str()) <= (int)max_w) {
            cur = next;
        } else {
            // 如果單一字就爆寬，硬塞進去避免死循環
            if (cur.empty()) {
                cur = add;
                flush();
            } else {
                flush();
                cur = add;
            }
        }
    }
    flush();
    return lines;
}

// ===== 如果超過 max_lines，就截斷並加 ... =====
static void clamp_lines_with_ellipsis(ALLEGRO_FONT* font,
                                      std::vector<std::string>& lines,
                                      int max_lines,
                                      float max_w) {
    if ((int)lines.size() <= max_lines) return;
    lines.resize(max_lines);

    // 最後一行尾巴加 ...
    std::string &last = lines.back();
    const char* ell = "...";
    while (!last.empty() &&
           al_get_text_width(font, (last + ell).c_str()) > (int)max_w) {
        last.pop_back();
    }
    last += ell;
}

void Phone::init() {

}

void Phone::update() {
    DataCenter* DC = DataCenter::get_instance();
    double now = al_get_time();

    // ===== 清掉過期通知 =====
    for (auto it = food_infos.begin(); it != food_infos.end(); ) {
        double age = now - it->create_time;
        if (age >= it->life_time) it = food_infos.erase(it);
        else ++it;
    }

    // ===== 換頁按鍵（只在 open 的時候）=====
    if (!open) return;

    bool left_pressed  = (DC->key_state[ALLEGRO_KEY_LEFT]  && !DC->prev_key_state[ALLEGRO_KEY_LEFT]);
    bool right_pressed = (DC->key_state[ALLEGRO_KEY_RIGHT] && !DC->prev_key_state[ALLEGRO_KEY_RIGHT]);

    if (left_pressed)  current_page--;
    if (right_pressed) current_page++;

    // clamp（total_pages_cached 會在 draw() 重新算）
    if (current_page < 0) current_page = 0;
    if (current_page > total_pages_cached - 1) current_page = total_pages_cached - 1;
}

void Phone::draw() {
    FontCenter *FC = FontCenter::get_instance();

    // 取得螢幕尺寸
    ALLEGRO_DISPLAY *disp = al_get_current_display();
    float dw = (float)al_get_display_width(disp);
    float dh = (float)al_get_display_height(disp);

    // ========== 背景遮罩 ==========
    al_draw_filled_rectangle(
        0, 0, dw, dh,
        al_map_rgba(0, 0, 0, 100)
    );

    // ========== 手機面板設定 ==========
    // 這裡我設計成右側豎立的一個長方形「手機」
    const float padding_screen = 20.0f;
    const float phone_w = 360.0f;
    const float phone_h = dh - padding_screen * 2.0f;
    const float phone_x = dw - phone_w - padding_screen;
    const float phone_y = padding_screen;

    // 手機背景（深灰＋微透明）
    al_draw_filled_rounded_rectangle(
        phone_x, phone_y,
        phone_x + phone_w, phone_y + phone_h,
        20.0f, 20.0f,
        al_map_rgba(32, 32, 32, 230)
    );

    // 手機邊框
    al_draw_rounded_rectangle(
        phone_x, phone_y,
        phone_x + phone_w, phone_y + phone_h,
        20.0f, 20.0f,
        al_map_rgb(255, 255, 255),
        2.0f
    );

    // ========== 標題 ==========
    ALLEGRO_FONT *title_font = FC->NotoSansCJK[FontSize::LARGE];
    ALLEGRO_FONT *text_font  = FC->NotoSansCJK[FontSize::SMALL];

    float padding_inner = 18.0f;
    float line_h = 24.0f;
    float cursor_y = phone_y + padding_inner;

    al_draw_text(
        title_font,
        al_map_rgb(255, 255, 255),
        phone_x + phone_w / 2.0f,
        cursor_y,
        ALLEGRO_ALIGN_CENTRE,
        "清交二手拍"
    );
    cursor_y += line_h * 2; // 留一點空白

    // Footer（留空間給分頁提示）
    const float footer_h = 44.0f;

    // 內容區
    float content_top = cursor_y;
    float content_bottom = phone_y + phone_h - padding_inner - footer_h;
    float content_h = content_bottom - content_top;

    // ===== 卡片排版參數 =====
    const float card_margin = 12.0f;
    const float card_pad = 12.0f;
    const float card_radius = 12.0f;

    const float text_left = phone_x + padding_inner;
    const float text_right = phone_x + phone_w - padding_inner;
    const float text_w = text_right - text_left;

    if (food_infos.empty()) {
        al_draw_text(
            text_font,
            al_map_rgb(220, 220, 220),
            text_left,
            content_top,
            ALLEGRO_ALIGN_LEFT,
            "目前沒有新的食物消息"
        );

        // footer
        al_draw_text(
            text_font,
            al_map_rgb(200,200,200),
            phone_x + phone_w / 2.0f,
            phone_y + phone_h - padding_inner - 24.0f,
            ALLEGRO_ALIGN_CENTRE,
            "← 上一頁   → 下一頁"
        );
        total_pages_cached = 1;
        current_page = 0;
        return;
    }

    // ===== 先把所有訊息切成「頁」：每頁塞到 content_h 塞不下就換頁 =====
    struct RenderItem {
        int idx; // food_infos index
        float card_h;
        std::vector<std::string> title_lines;
        std::vector<std::string> content_lines;
    };

    std::vector<std::vector<RenderItem>> pages;
    pages.emplace_back();
    float y_used = 0.0f; // 當前頁已用高度

    for (int i = 0; i < (int)food_infos.size(); ++i) {
        const auto &info = food_infos[i];

        // title: [建築] message
        std::string title = "[" + info.building_name + "] " + info.message;
        auto title_lines = wrap_text(text_font, title, text_w - card_pad * 2);
        clamp_lines_with_ellipsis(text_font, title_lines, 2, text_w - card_pad * 2); // title 最多 2 行

        // content
        auto content_lines = wrap_text(text_font, info.content, text_w - card_pad * 2 - 12);
        clamp_lines_with_ellipsis(text_font, content_lines, 3, text_w - card_pad * 2 - 12); // content 最多 3 行

        int total_lines = (int)title_lines.size() + (int)content_lines.size();
        float card_h = card_pad * 2 + total_lines * line_h + 6.0f; // +6 微調

        // 看塞不塞得下
        float need = (y_used == 0 ? 0 : card_margin) + card_h;
        if (y_used + need > content_h) {
            pages.emplace_back();
            y_used = 0.0f;
            need = card_h;
        }

        RenderItem item;
        item.idx = i;
        item.card_h = card_h;
        item.title_lines = std::move(title_lines);
        item.content_lines = std::move(content_lines);

        if (y_used != 0) y_used += card_margin;
        pages.back().push_back(std::move(item));
        y_used += card_h;
    }

    // 更新總頁數 cache + clamp current_page
    total_pages_cached = std::max(1, (int)pages.size());
    if (current_page < 0) current_page = 0;
    if (current_page > total_pages_cached - 1) current_page = total_pages_cached - 1;

    // ===== 畫當前頁 =====
    float y = content_top;
    const auto &page = pages[current_page];

    for (const auto &it : page) {
        // card bg
        float card_x1 = phone_x + padding_inner;
        float card_x2 = phone_x + phone_w - padding_inner;
        float card_y1 = y;
        float card_y2 = y + it.card_h;

        al_draw_filled_rounded_rectangle(
            card_x1, card_y1, card_x2, card_y2,
            card_radius, card_radius,
            al_map_rgba(20, 20, 20, 180)
        );
        al_draw_rounded_rectangle(
            card_x1, card_y1, card_x2, card_y2,
            card_radius, card_radius,
            al_map_rgba(255,255,255,60),
            1.5f
        );

        float tx = card_x1 + card_pad;
        float ty = card_y1 + card_pad;

        // title lines
        for (auto &ln : it.title_lines) {
            al_draw_text(
                text_font,
                al_map_rgb(235,235,235),
                tx,
                ty,
                ALLEGRO_ALIGN_LEFT,
                ln.c_str()
            );
            ty += line_h;
        }

        // content lines（縮排）
        float cx = tx + 12;
        for (auto &ln : it.content_lines) {
            al_draw_text(
                text_font,
                al_map_rgb(200,200,200),
                cx,
                ty,
                ALLEGRO_ALIGN_LEFT,
                ln.c_str()
            );
            ty += line_h;
        }

        y += it.card_h + card_margin;
    }

    // ===== footer：頁碼 + 操作提示 =====
    char pagebuf[64];
    std::snprintf(pagebuf, sizeof(pagebuf), "Page %d / %d", current_page + 1, total_pages_cached);

    al_draw_text(
        text_font,
        al_map_rgb(200,200,200),
        phone_x + padding_inner,
        phone_y + phone_h - padding_inner - 24.0f,
        ALLEGRO_ALIGN_LEFT,
        pagebuf
    );

    al_draw_text(
        text_font,
        al_map_rgb(200,200,200),
        phone_x + phone_w - padding_inner,
        phone_y + phone_h - padding_inner - 24.0f,
        ALLEGRO_ALIGN_RIGHT,
        "A/← 上一頁   D/→ 下一頁"
    );
}

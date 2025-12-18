#ifndef MONSTER_H_INCLUDED 
#define MONSTER_H_INCLUDED

#include "../Object.h"
#include "../shapes/Rectangle.h"
#include "../shapes/Point.h"
#include "../object/Build.h"
#include "../Build_object/Build_A.h"
#include "../Build_object/Build_B.h"
#include <vector>
#include <queue>

enum class Dir;

// ===== 怪物種類 =====
enum class MonsterType {
    WOLF,
    CAVEMAN,
    WOLFKNIGHT,
    DEMONNIJIA,
    MONSTERTYPE_MAX
};

// ===== 怪物 AI 狀態：亂走 / 去建築物搶學餐 =====
enum class AIState {
    WANDER,         // 隨機亂走
    GO_TO_BUILDING, // 朝有食物的建築物前進
    GO_TO_BUILDINGB
};

/**
 * @brief The class of a monster (enemies / NPC 學餐搶手).
 * @details Monster inherits Object and takes Rectangle as its hit box.
 */
class Monster : public Object
{
public:
    // 建立對應 type 的 monster 實例
    static Monster *create_monster(MonsterType type, const std::vector<Point> &path);

public:
    // path 只拿來決定初始位置，不再用來當完整行走路徑
    Monster(const std::vector<Point> &path, MonsterType type);

    void update();          // ★ 這裡不要 override，因為 Object 沒有 virtual update()
    void draw() override;   // draw 有在 Object 宣告成 virtual，所以可以 override

    void on_hit_by_bullet();  // 子彈命中時呼叫
    bool is_alive() const { return alive; }

    const int &get_money() const { return money; }
    int HP;

protected:
    int v;
    int money;
    std::vector<std::vector<int>> bitmap_img_ids;
    int bitmap_switch_counter;
    int bitmap_switch_freq;
    int bitmap_img_id;

    bool alive = true;
    double respawn_timer = 0.0;      // 秒
    double spawn_x = 0.0, spawn_y = 0.0; // 初始出生點(世界座標)

private:
    MonsterType type;
    Dir dir;

    // === AI 狀態：亂走 / 去建築物 ===
    AIState ai_state = AIState::WANDER;

    // 目前移動速度向量（像素／秒）
    double vx = 0.0;
    double vy = 0.0;

    // 亂走模式下，還要持續目前方向多久（秒）
    double wander_timer = 0.0;

    // 目前鎖定要去搶的建築物（有食物的 Build_A）
    Build_A *target_building = nullptr;
    Build_B *target_building_B = nullptr;

    // 追建築物分兩階段：0 = 先走 X, 1 = 再走 Y
    int chase_phase = 0;

    // 從亂走狀態中挑一個新的方向與速度
    void choose_random_direction();
};

#endif

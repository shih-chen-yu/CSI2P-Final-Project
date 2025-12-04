#ifndef MONSTER_H_INCLUDED 
#define MONSTER_H_INCLUDED

#include "../Object.h"
#include "../shapes/Rectangle.h"
#include "../shapes/Point.h"
#include <vector>
#include <queue>

enum class Dir;
class Building;   // 前向宣告，讓 Monster 可以持有 Building*

// fixed settings
enum class MonsterType {
    WOLF, CAVEMAN, WOLFKNIGHT, DEMONNIJIA, MONSTERTYPE_MAX
};

/**
 * @brief The class of a monster (enemies).
 * @details Monster inherits Object and takes Rectangle as its hit box.
 */
class Monster : public Object
{
public:
    static Monster *create_monster(MonsterType type, const std::vector<Point> &path);

public:
    // path 只拿來決定初始位置，不再用來當完整行走路徑
    Monster(const std::vector<Point> &path, MonsterType type);

    void update();
    void draw();

    const int &get_money() const { return money; }
    int HP;

    // 原本回傳 path 的介面現在不再需要，可以移除或保留空實作
    // const std::queue<Point> &get_path() const { return path; }

protected:
    /**
     * @var HP
     * @brief Health point of a monster.
     **
     * @var v
     * @brief Moving speed of a monster (pixels per second).
     **
     * @var money
     * @brief The amount of money that player will earn when the monster is killed.
     **
     * @var bitmap_img_ids
     * @brief The first vector is the Dir index, and the second vector is image id.
     * @details `bitmap_img_ids[Dir][<ordered_id>]`
     **
     * @var bitmap_switch_counter
     * @brief Counting down for bitmap_switch_freq.
     * @see Monster::bitmap_switch_freq
     **
     * @var bitmap_switch_freq
     * @brief Number of frames required to change to the next move pose for the current facing direction.
     * @details The variable is left for child classes to define.
     * 
     * @var bitmap_img_id
     * @brief Move pose of the current facing direction.
     **
     * @var dir
     * @brief Current facing direction.
     */
    int v;
    int money;
    std::vector<std::vector<int>> bitmap_img_ids;
    int bitmap_switch_counter;
    int bitmap_switch_freq;
    int bitmap_img_id;

private:
    // === AI 狀態：亂走 / 去建築物 ===
    enum class AIState {
        WANDER,         // 隨機亂走
        GO_TO_BUILDING  // 朝有驚嘆號的建築物前進
    };

    MonsterType type;
    Dir dir;

    // 亂走 & 追建築物用的狀態
    AIState ai_state = AIState::WANDER;

    // 目前移動速度向量（像素／秒）
    double vx = 0.0;
    double vy = 0.0;

    // 亂走模式下，還要持續目前方向多久（秒）
    double wander_timer = 0.0;

    // 目前鎖定要去的建築物（如果有）
    Building *target_building = nullptr;

    // 追建築物分兩階段：0 = 先走 X, 1 = 再走 Y
    int chase_phase = 0;

    // 從亂走狀態中挑一個新的方向與速度
    void choose_random_direction();
};

#endif

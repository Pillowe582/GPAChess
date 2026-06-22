#ifndef OVERWHHELMED_BRO_ALLY_H
#define OVERWHHELMED_BRO_ALLY_H
#include "entity/ally_behavior.h"

class GameManager;

/// OverwhelmedBro 我方：远程，找最远敌人，发射子弹，不移动
class OverwhelmedBroAlly : public AllyBehavior
{
public:
    void tick(double dt, BaseEntity &self, GameManager &gameManager) override;

    void onStart(AllyInstance &self);

private:
    // ---- 此角色自有的子弹数据 ----
    struct Bullet
    {
        double x = 0, y = 0;
        double vx = 0, vy = 0;
        double rot = 0;
        int damage = 0;
    };
    std::vector<Bullet> m_bullets;
    double m_maxCooldown = 1.5; // 发射冷却
    double m_currentCooldown = 0.0;

    // ---- 拆分的子方法 ----

    /// 推进并渲染已有子弹，检测碰撞，移除失效子弹
    void updateBullets(double dt,
                       std::vector<EnemyInstance *> &enemies,
                       Renderer &renderer,
                       AllyInstance &self,
                       GameManager &gameManager);

    /// 找最远的存活敌人，返回 nullptr 表示没找到
    EnemyInstance *findFarthestEnemy(const std::vector<EnemyInstance *> &enemies,
                                     const AllyInstance &self);

    /// 向目标发射一颗子弹，存入 m_bullets
    void fireBullet(const AllyInstance &self, EnemyInstance *target);
};

#endif
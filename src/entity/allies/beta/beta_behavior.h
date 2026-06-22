#ifndef BETA_ALLY_H
#define BETA_ALLY_H
#include "entity/ally_behavior.h"

class GameManager;

/// Beta 我方：远程，找最远敌人，发射子弹，不移动
class BetaAlly : public AllyBehavior
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
        int damage = 0;
    };
    std::vector<Bullet> m_bullets;
    double m_cooldown = 0.0;
};

#endif
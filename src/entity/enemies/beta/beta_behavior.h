#ifndef BETA_ENEMY_H
#define BETA_ENEMY_H
#include "entity/enemy_behavior.h"
#include <memory>

class ChessInstance;

/// Beta 敌方：远程，找最远我方单位，发射子弹；无目标时攻击塔
class BetaEnemy : public EnemyBehavior
{
public:
    void tick(double dt, EnemyInstance &self,
              std::vector<std::unique_ptr<ChessInstance>> &allies,
              Renderer &renderer,
              int &pendingGold, int &pendingExp) override;

    void onStart(EnemyInstance &self);

private:
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

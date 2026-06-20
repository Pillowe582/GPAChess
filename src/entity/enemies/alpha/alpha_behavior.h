#ifndef ALPHA_ENEMY_H
#define ALPHA_ENEMY_H
#include "entity/enemy_behavior.h"

/// Alpha 敌方：近战，找最近我方单位，移动到70px后3圈武器挥砍
class AlphaEnemy : public EnemyBehavior
{
public:
    void tick(double dt, EnemyInstance &self,
              std::vector<ChessInstance> &allies,
              std::vector<DrawCmd> &draws,
              int &towerHp, int &pendingGold, int &pendingExp,
              const SplashFn &splash) override;

private:
    struct Weapon
    {
        bool active = false;
        double angle = 0.0;
        double elapsed = 0.0;
        int rotationsDone = 0;
        int targetUuid = -1;
    };
    Weapon m_weapon;
    double m_cooldown = 0.0;
};

#endif

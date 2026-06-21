#ifndef ALPHA_ENEMY_H
#define ALPHA_ENEMY_H
#include "entity/enemy_behavior.h"
#include <memory>

class ChessInstance;

/// Alpha 敌方：近战，找最近我方单位，移动到70px后3圈武器挥砍
class AlphaEnemy : public EnemyBehavior
{
public:
    void tick(double dt, EnemyInstance &self,
              std::vector<std::unique_ptr<ChessInstance>> &allies,
              Renderer &renderer,
              int &pendingGold, int &pendingExp) override;

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

#ifndef TOWER_BEHAVIOR_H
#define TOWER_BEHAVIOR_H

#include "entity/base_behavior.h"
#include <vector>

class EnemyInstance;
class Renderer;

class TowerBehavior : public BaseBehavior
{
public:
    // ========== 逻辑 tick ==========
    virtual void tick(double dt,
                      int &currentHp,
                      int maxHp,
                      double &cooldown,
                      std::vector<EnemyInstance> &enemies,
                      Renderer &renderer,
                      int &pendingGold,
                      int &pendingExp) = 0;

    // ========== 渲染 tick（与逻辑分离）==========
    virtual void renderSelf(int currentHp, int maxHp, Renderer &r);
};

TowerBehavior *createTowerBehavior();

#endif

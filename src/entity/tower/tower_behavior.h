#ifndef TOWER_BEHAVIOR_H
#define TOWER_BEHAVIOR_H

#include "entity/base_behavior.h"
#include <vector>

class EnemyInstance;
class Renderer;

class TowerBehavior : public BaseBehavior
{
public:
    virtual void tick(double dt,
                      int &currentHp,
                      int maxHp,
                      double &cooldown,
                      std::vector<EnemyInstance> &enemies,
                      Renderer &renderer,
                      int &pendingGold,
                      int &pendingExp) = 0;
};

TowerBehavior *createTowerBehavior();

#endif

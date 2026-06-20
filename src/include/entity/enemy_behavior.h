#ifndef ENEMY_BEHAVIOR_H
#define ENEMY_BEHAVIOR_H

#include "entity/base_behavior.h"
#include <vector>

class ChessInstance;
class EnemyInstance;
class Renderer;

class EnemyBehavior : public BaseBehavior
{
public:
    // ========== 逻辑 tick ==========
    virtual void tick(double dt,
                      EnemyInstance &self,
                      std::vector<ChessInstance> &allies,
                      Renderer &renderer,
                      int &towerHp,
                      int &pendingGold,
                      int &pendingExp) = 0;

    // ========== 渲染 tick（与逻辑分离）==========
    virtual void renderSelf(const EnemyInstance &self, Renderer &r);
};

EnemyBehavior *createEnemyBehavior(int behaviorId);

#endif

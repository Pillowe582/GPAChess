#ifndef ENEMY_BEHAVIOR_H
#define ENEMY_BEHAVIOR_H

#include "entity/base_behavior.h"
#include <vector>
#include <memory>

class ChessInstance;
class EnemyInstance;
class Renderer;

class EnemyBehavior : public BaseBehavior
{
public:
    // ========== 逻辑 tick ==========
    virtual void tick(double dt,
                      EnemyInstance &self,
                      std::vector<std::unique_ptr<ChessInstance>> &allies,
                      Renderer &renderer,
                      int &pendingGold,
                      int &pendingExp) = 0;

    /// 回合开始时调用，用于初始化技能冷却等状态
    virtual void onStart(EnemyInstance & /*self*/) { /* 默认空实现 */ }

    // ========== 渲染 tick（与逻辑分离）==========
    virtual void renderSelf(const EnemyInstance &self, Renderer &r,
                            double x, double y);
};

EnemyBehavior *createEnemyBehavior(int behaviorId);

#endif

#ifndef ENEMY_BEHAVIOR_H
#define ENEMY_BEHAVIOR_H

#include "entity/base_behavior.h"
#include <vector>
#include <memory>

class AllyInstance;
class EnemyInstance;
class Renderer;
class GameManager;

class EnemyBehavior : public BaseBehavior
{
public:
    void tick(double dt, BaseEntity &self, GameManager &gameManager) override = 0;

    /// 回合开始时调用，用于初始化技能冷却等状态
    virtual void onStart(EnemyInstance & /*self*/) { /* 默认空实现 */ }

    // ========== 渲染 tick（与逻辑分离）==========
    virtual void renderSelf(const EnemyInstance &self, Renderer &r,
                            double x, double y);
};

EnemyBehavior *createEnemyBehavior(int behaviorId);

#endif

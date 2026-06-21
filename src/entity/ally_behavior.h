#ifndef ALLY_BEHAVIOR_H
#define ALLY_BEHAVIOR_H

#include "entity/base_behavior.h"
#include <vector>

class ChessInstance;
class EnemyInstance;
class Renderer;
class QPainter;

class AllyBehavior : public BaseBehavior
{
public:
    virtual void tick(double dt,
                      ChessInstance &self,
                      std::vector<EnemyInstance> &enemies,
                      Renderer &renderer,
                      int &pendingGold,
                      int &pendingExp) = 0;

    /// 回合开始时调用，用于初始化技能冷却等状态
    virtual void onStart(ChessInstance & /*self*/) { /* 默认空实现 */ }

    /// Queue 渲染 —— refreshAllUnits 调用，通过 Renderer 队列绘制
    virtual void renderSelf(const ChessInstance &self, Renderer &r,
                            double x, double y);

    /// Paint 渲染 —— Qt paint 事件驱动，用 QPainter 直接绘制
    virtual void renderSelf(const ChessInstance &self, QPainter &p,
                            double radius);
};

AllyBehavior *createAllyBehavior(int behaviorId);

#endif

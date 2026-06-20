#ifndef ALLY_BEHAVIOR_H
#define ALLY_BEHAVIOR_H

#include "entity/base_behavior.h"
#include <vector>

class ChessInstance;
class EnemyInstance;
class Renderer;

class AllyBehavior : public BaseBehavior
{
public:
    virtual void tick(double dt,
                      ChessInstance &self,
                      std::vector<EnemyInstance> &enemies,
                      Renderer &renderer,
                      int &pendingGold,
                      int &pendingExp) = 0;

    /// 渲染自身 —— 由 refreshAllUnits 每帧调用
    /// entry 负责计算坐标 (x,y), behavior 读取 self 的状态决定画什么
    virtual void renderSelf(const ChessInstance &self, Renderer &r,
                            double x, double y);
};

AllyBehavior *createAllyBehavior(int behaviorId);

#endif

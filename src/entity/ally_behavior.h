#ifndef ALLY_BEHAVIOR_H
#define ALLY_BEHAVIOR_H

#include "entity/base_behavior.h"
#include <vector>
#include <QString>

class AllyInstance;
class EnemyInstance;
class Renderer;
class GameManager;
class QPainter;
class BaseEntity;

class AllyBehavior : public BaseBehavior
{
public:
    void tick(double dt, BaseEntity &self, GameManager &gameManager) override = 0;

    /// 回合开始时调用，用于初始化技能冷却等状态
    virtual void onStart(AllyInstance & /*self*/) { /* 默认空实现 */ }

    /// Queue 渲染 —— refreshAllUnits 调用，通过 Renderer 队列绘制
    virtual void renderSelf(const AllyInstance &self, Renderer &r,
                            double x, double y);

    /// Paint 渲染 —— Qt paint 事件驱动，用 QPainter 直接绘制
    virtual void renderSelf(const AllyInstance &self, QPainter &p,
                            double radius);

    /// @brief 获取材质，默认取同名材质
    /// @param self 棋子实例
    /// @return 材质路径
    virtual QString getTexturePath(const AllyInstance &self) const;
};

AllyBehavior *createAllyBehavior(int behaviorId);

#endif

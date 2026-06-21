#ifndef TOWER_BEHAVIOR_H
#define TOWER_BEHAVIOR_H

#include "ally_behavior.h"

/// 防御塔行为 —— 继承 AllyBehavior，塔就是一个不能移动的友方单位
class TowerBehavior : public AllyBehavior
{
public:
    void tick(double dt,
              ChessInstance &self,
              std::vector<EnemyInstance> &enemies,
              Renderer &renderer,
              int &pendingGold,
              int &pendingExp) override;

    void renderSelf(const ChessInstance &self, Renderer &r,
                    double x, double y) override;
    void renderSelf(const ChessInstance &self, QPainter &p,
                    double radius) override;

private:
    double m_attackCooldown = 0.0;
};

#endif

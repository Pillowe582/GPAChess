#ifndef TOWER_BEHAVIOR_H
#define TOWER_BEHAVIOR_H

#include "ally_behavior.h"
#include <QPen>

class GameManager;

/// 防御塔行为 —— 继承 AllyBehavior，塔就是一个不能移动的友方单位
class TowerBehavior : public AllyBehavior
{
public:
    void tick(double dt, BaseEntity &self, GameManager &gameManager) override;

    void onStart(AllyInstance &self);

    void renderSelf(const AllyInstance &self, Renderer &r,
                    double x, double y) override;
    void renderSelf(const AllyInstance &self, QPainter &p,
                    double radius) override {};

    void renderLaserAnimation(Renderer &renderer, AllyInstance &self, EnemyInstance &target);

private:
    double m_maxAttackCooldown = 1.5; // 塔的攻击间隔为1.5秒
    double m_currentAttackCooldown = 0.0;
    double m_hpRatio = 1.0;
    EnemyInstance *m_laserTarget = nullptr; // 当前激光目标
    double m_laserTimer = 0.0;              // 激光动画剩余时间
    /// @brief 执行一次攻击逻辑
    void attack(double dt, AllyInstance &self, GameManager &gameManager);
    QPen m_laserPen;
};

#endif
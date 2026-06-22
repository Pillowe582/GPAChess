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

    void onStart(ChessInstance &self);

    void renderSelf(const ChessInstance &self, Renderer &r,
                    double x, double y) override;
    void renderSelf(const ChessInstance &self, QPainter &p,
                    double radius) override {};

private:
    double m_maxAttackCooldown = 1.5; // 塔的攻击间隔为1.5秒
    double m_currentAttackCooldown = 0.0;
    double m_hpRatio = 1.0;

    /// @brief 执行一次攻击逻辑
    /// @param self 自身
    /// @param enemies 敌人列表
    /// @param pendingGold 待发放金币（回传）
    /// @param pendingExp 待发放经验值（回传）
    void attack(double dt, ChessInstance &self, std::vector<EnemyInstance> &enemies,
                int &pendingGold, int &pendingExp);
};

#endif

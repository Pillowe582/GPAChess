#ifndef ALPHA_ALLY_H
#define ALPHA_ALLY_H
#include "entity/ally_behavior.h"

/// Alpha 我方：近战，找最近敌人，移动到70px距离后3圈武器挥砍
class AlphaAlly : public AllyBehavior
{
public:
    void tick(double dt, ChessInstance &self,
              std::vector<EnemyInstance> &enemies,
              Renderer &renderer,
              int &pendingGold, int &pendingExp) override;

private:
    // ---- 此角色自有的武器状态 ----
    struct Weapon
    {
        bool active = false;
        double angle = 0.0;    // 当前旋转角度（弧度）
        double elapsed = 0.0;  // 已过时间
        int rotationsDone = 0; // 已完成圈数
        int targetUuid = -1;   // 目标uuid
    };
    Weapon m_weapon;
    double m_cooldown = 0.0;
};

#endif

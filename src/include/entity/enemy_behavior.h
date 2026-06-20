#ifndef ENEMY_BEHAVIOR_H
#define ENEMY_BEHAVIOR_H

#include <QColor>
#include <QString>
#include <functional>
#include <vector>
#include "ally_behavior.h"

class ChessInstance;
class EnemyInstance;

/// 敌方单位行为基类 —— 每个角色完全自包含
class EnemyBehavior
{
public:
    virtual ~EnemyBehavior() = default;

    using SplashFn = std::function<void(const QString &, double, double, const QString &)>;

    /// 每帧主逻辑：移动、攻击、生成渲染数据
    virtual void tick(double dt,
                      EnemyInstance &self,
                      std::vector<ChessInstance> &allies,
                      std::vector<DrawCmd> &draws,
                      int &towerHp,
                      int &pendingGold,
                      int &pendingExp,
                      const SplashFn &splash) = 0;
};

EnemyBehavior *createEnemyBehavior(int behaviorId);

#endif

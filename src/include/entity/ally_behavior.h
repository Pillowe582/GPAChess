#ifndef ALLY_BEHAVIOR_H
#define ALLY_BEHAVIOR_H

#include <QColor>
#include <QString>
#include <functional>
#include <vector>

class ChessInstance;
class EnemyInstance;
struct DrawCmd; // 前置声明，定义在 state.h

/// 我方单位行为基类 —— 每个角色完全自包含
class AllyBehavior
{
public:
    virtual ~AllyBehavior() = default;

    using SplashFn = std::function<void(const QString &, double, double, const QString &)>;

    /// 每帧主逻辑：移动、攻击、生成渲染数据，全部在此完成
    virtual void tick(double dt,
                      ChessInstance &self,
                      std::vector<EnemyInstance> &enemies,
                      std::vector<DrawCmd> &draws,
                      int &pendingGold,
                      int &pendingExp,
                      const SplashFn &splash) = 0;
};

AllyBehavior *createAllyBehavior(int behaviorId);

#endif

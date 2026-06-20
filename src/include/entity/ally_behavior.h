#ifndef ALLY_BEHAVIOR_H
#define ALLY_BEHAVIOR_H

#include <QString>
#include <vector>

class ChessInstance;
class EnemyInstance;

/// 我方单位行为基类（策略模式）
class AllyBehavior
{
public:
    virtual ~AllyBehavior() = default;
    virtual EnemyInstance *findTarget(std::vector<EnemyInstance> &enemies,
                                      const ChessInstance &self) = 0;
    virtual int getDamage(const ChessInstance &self) const = 0;
    virtual double getInterval(const ChessInstance &self) const = 0;
    virtual QString damageColor() const = 0;
};

/// 工厂：1=Alpha(近), 2=Beta(远)
AllyBehavior *createAllyBehavior(int behaviorId);

#endif

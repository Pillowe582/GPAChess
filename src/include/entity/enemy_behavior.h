#ifndef ENEMY_BEHAVIOR_H
#define ENEMY_BEHAVIOR_H

#include <QString>
#include <vector>

class ChessInstance;
class EnemyInstance;

/// 敌方单位行为基类
class EnemyBehavior
{
public:
    virtual ~EnemyBehavior() = default;
    virtual ChessInstance *findTarget(std::vector<ChessInstance> &allies,
                                      const EnemyInstance &self) = 0;
    virtual int getDamage(const EnemyInstance &self) const = 0;
    virtual double getInterval(const EnemyInstance &self) const = 0;
    virtual QString damageColor() const = 0;
};

EnemyBehavior *createEnemyBehavior(int behaviorId);

#endif

#ifndef ALPHA_ENEMY_H
#define ALPHA_ENEMY_H
#include "entity/enemy_behavior.h"

class AlphaEnemy : public EnemyBehavior
{
public:
    ChessInstance *findTarget(std::vector<ChessInstance> &allies,
                              const EnemyInstance &self) override;
    int getDamage(const EnemyInstance &self) const override;
    double getInterval(const EnemyInstance &self) const override;
    QString damageColor() const override;
};
#endif

#ifndef ALPHA_ALLY_H
#define ALPHA_ALLY_H
#include "entity/ally_behavior.h"

class AlphaAlly : public AllyBehavior
{
public:
    EnemyInstance *findTarget(std::vector<EnemyInstance> &enemies,
                              const ChessInstance &self) override;
    int getDamage(const ChessInstance &self) const override;
    double getInterval(const ChessInstance &self) const override;
    QString damageColor() const override;
};
#endif

#include "beta_behavior.h"
#include "state.h"
#include <cmath>

ChessInstance *BetaEnemy::findTarget(std::vector<ChessInstance> &allies,
                                     const EnemyInstance &self)
{
    ChessInstance *best = nullptr;
    double bestDist = -1.0;
    for (auto &a : allies)
    {
        if (!a.isAlive || !a.deployed)
            continue;
        double dx = a.posX - self.posX;
        double dy = a.posY - self.posY;
        double d = std::sqrt(dx * dx + dy * dy);
        if (d > bestDist)
        {
            bestDist = d;
            best = &a;
        }
    }
    return best;
}

int BetaEnemy::getDamage(const EnemyInstance &self) const { return self.atk.getFinal(); }
double BetaEnemy::getInterval(const EnemyInstance &self) const
{
    int spd = self.attackSpeed.getFinal();
    return spd > 0 ? 1.0 / spd : 1.0;
}
QString BetaEnemy::damageColor() const { return "#FFFFFF"; }

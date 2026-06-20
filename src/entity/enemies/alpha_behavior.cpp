#include "alpha_behavior.h"
#include "beta_behavior.h"
#include "state.h"
#include <cmath>

ChessInstance *AlphaEnemy::findTarget(std::vector<ChessInstance> &allies,
                                      const EnemyInstance &self)
{
    ChessInstance *best = nullptr;
    double bestDist = 1e18;
    for (auto &a : allies)
    {
        if (!a.isAlive || !a.deployed)
            continue;
        double dx = a.posX - self.posX;
        double dy = a.posY - self.posY;
        double d = std::sqrt(dx * dx + dy * dy);
        if (d < bestDist)
        {
            bestDist = d;
            best = &a;
        }
    }
    return best;
}

int AlphaEnemy::getDamage(const EnemyInstance &self) const { return self.atk.getFinal(); }
double AlphaEnemy::getInterval(const EnemyInstance &self) const
{
    int spd = self.attackSpeed.getFinal();
    return spd > 0 ? 1.0 / spd : 1.0;
}
QString AlphaEnemy::damageColor() const { return "#FFFFFF"; }

// ---- 工厂 ----
EnemyBehavior *createEnemyBehavior(int behaviorId)
{
    switch (behaviorId)
    {
    case 1:
        return new AlphaEnemy();
    case 2:
        return new BetaEnemy();
    default:
        return new AlphaEnemy();
    }
}

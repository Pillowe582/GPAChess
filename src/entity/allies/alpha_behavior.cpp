#include "alpha_behavior.h"
#include "beta_behavior.h"
#include "state.h"
#include <cmath>

EnemyInstance *AlphaAlly::findTarget(std::vector<EnemyInstance> &enemies,
                                     const ChessInstance &self)
{
    EnemyInstance *best = nullptr;
    double bestDist = 1e18;
    for (auto &e : enemies)
    {
        if (!e.isAlive)
            continue;
        double dx = e.posX - self.posX;
        double dy = e.posY - self.posY;
        double d = std::sqrt(dx * dx + dy * dy);
        if (d < bestDist)
        {
            bestDist = d;
            best = &e;
        }
    }
    return best;
}

int AlphaAlly::getDamage(const ChessInstance &self) const { return self.atk.getFinal(); }
double AlphaAlly::getInterval(const ChessInstance &self) const
{
    int spd = self.attackSpeed.getFinal();
    return spd > 0 ? 1.0 / spd : 1.0;
}
QString AlphaAlly::damageColor() const { return "#FF8C32"; }

// ---- 工厂 ----
AllyBehavior *createAllyBehavior(int behaviorId)
{
    switch (behaviorId)
    {
    case 1:
        return new AlphaAlly();
    case 2:
        return new BetaAlly();
    default:
        return new AlphaAlly();
    }
}

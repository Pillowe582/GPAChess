#include "beta_behavior.h"
#include "state.h"
#include <cmath>

EnemyInstance *BetaAlly::findTarget(std::vector<EnemyInstance> &enemies,
                                    const ChessInstance &self)
{
    EnemyInstance *best = nullptr;
    double bestDist = -1.0;
    for (auto &e : enemies)
    {
        if (!e.isAlive)
            continue;
        double dx = e.posX - self.posX;
        double dy = e.posY - self.posY;
        double d = std::sqrt(dx * dx + dy * dy);
        if (d > bestDist)
        {
            bestDist = d;
            best = &e;
        }
    }
    return best;
}

int BetaAlly::getDamage(const ChessInstance &self) const { return self.atk.getFinal(); }
double BetaAlly::getInterval(const ChessInstance &self) const
{
    int spd = self.attackSpeed.getFinal();
    return spd > 0 ? 1.0 / spd : 1.0;
}
QString BetaAlly::damageColor() const { return "#64DC50"; }

#include "entity/ally_behavior.h"
#include "state.h"
#include "render/renderer.h"
#include "allies/alpha/alpha_behavior.h"
#include "allies/beta/beta_behavior.h"

#include <algorithm>

/// 默认渲染：圆形 + 血条 + 名字/HP
void AllyBehavior::renderSelf(const ChessInstance &self, Renderer &r,
                              double x, double y)
{
    double radius = self.deployed ? 34.0 : 22.0;
    QColor bodyC = self.deployed ? QColor(60, 110, 255) : QColor(70, 120, 255);

    r.queueCircle(x, y, radius, bodyC, 10);

    int maxHp = self.hp.getFinal();
    if (maxHp <= 0)
        return;
    double hpRatio = std::clamp(static_cast<double>(self.currentHp) / maxHp, 0.0, 1.0);

    double barW = radius * 3.0, barH = 8.0;
    double barX = x - barW / 2.0, barY = y - radius - 24.0;

    r.queueRect(barX, barY, barW, barH, QColor(40, 40, 40, 220), 20);
    r.queueRect(barX, barY, barW * hpRatio, barH, QColor(60, 160, 255), 21);

    r.queueText(QString("%1\n%2/%3").arg(self.name).arg(self.currentHp).arg(maxHp),
                barX, barY + barH + 4.0, QColor(0, 0, 0, 160), 29);
    r.queueText(QString("%1\n%2/%3").arg(self.name).arg(self.currentHp).arg(maxHp),
                barX - 1.0, barY + barH + 3.0, Qt::white, 30);
}

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

#include "entity/enemy_behavior.h"
#include "state.h"
#include "render/renderer.h"
#include "enemies/alpha/alpha_behavior.h"
#include "enemies/beta/beta_behavior.h"

#include <algorithm>

/// 默认敌人渲染：红色圆形 + 血条 + 名字/HP
void EnemyBehavior::renderSelf(const EnemyInstance &self, Renderer &r)
{
    double x = self.posX, y = self.posY, radius = 34.0;
    r.queueCircle(x, y, radius, QColor(240, 70, 70), 10);

    double barW = radius * 3.0, barH = 8.0;
    double barX = x - barW / 2.0, barY = y - radius - 24.0;
    r.queueRect(barX, barY, barW, barH, QColor(40, 40, 40, 220), 20);

    int maxHp = self.hp.getFinal();
    double hpRatio = maxHp > 0 ? std::clamp(static_cast<double>(self.currentHp) / maxHp, 0.0, 1.0) : 0.0;
    r.queueRect(barX, barY, barW * hpRatio, barH, QColor(220, 60, 60), 21);

    r.queueText(QString("%1\n%2/%3").arg(self.name).arg(self.currentHp).arg(maxHp),
                barX, barY + barH + 4.0, QColor(0, 0, 0, 160), 29);
    r.queueText(QString("%1\n%2/%3").arg(self.name).arg(self.currentHp).arg(maxHp),
                barX - 1.0, barY + barH + 3.0, Qt::white, 30);
}

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

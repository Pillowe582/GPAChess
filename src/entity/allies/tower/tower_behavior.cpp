#include "allies/tower/tower_behavior.h"
#include "state.h"
#include "renderer.h"

#include <algorithm>
#include <QRandomGenerator>
#include <QPainter>
#include <QPen>
#include <QFont>

// ========== 逻辑 tick ==========
void TowerBehavior::tick(double dt,
                         ChessInstance &self,
                         std::vector<EnemyInstance> &enemies,
                         Renderer &renderer,
                         int &pendingGold,
                         int &pendingExp)
{
    auto rng = QRandomGenerator::global();
    auto jitter = [rng]() -> double
    { return (rng->generateDouble() - 0.5) * 30.0; };

    m_attackCooldown = std::max(0.0, m_attackCooldown - dt);
    if (m_attackCooldown > 0.0)
        return;

    // 找第一个存活敌人
    auto it = std::find_if(enemies.begin(), enemies.end(),
                           [](const EnemyInstance &e)
                           { return e.isAlive; });
    if (it == enemies.end())
        return;

    // 伤害 = 30 × (1 + (1 - HP%) × 3)
    double hpRatio = self.maxHp.getFinal() > 0
                         ? self.currentHp / self.maxHp.getFinal()
                         : 1.0;
    int dmg = static_cast<int>(30.0 * (1.0 + (1.0 - hpRatio) * 3.0));

    it->dealDamage(dmg, DamageType{DamageType::Physical, QColor("#4696FF")});
    renderer.queueSplash(QString("-%1").arg(dmg),
                         it->transform.x + jitter(),
                         it->transform.y - 20 + jitter(),
                         QColor("#4696FF"));

    if (!it->isAlive)
    {
        if (rng->bounded(10) == 0)
            pendingGold += it->baseGoldReward;
        pendingExp += it->baseExpReward;
    }

    m_attackCooldown = 1.5;
}

// ========== Queue 渲染 ==========
void TowerBehavior::renderSelf(const ChessInstance &self, Renderer &r,
                               double x, double y)
{
    const double tx = 10.0, ts = 70.0, ty = 400.0 - ts / 2.0;
    r.queueRect(tx, ty, ts, ts, QColor(40, 100, 220, 200), 5);

    r.queueRect(tx, ty - 12.0, ts, 6.0, QColor(40, 40, 40, 200), 6);

    double hpRatio = self.maxHp.getFinal() > 0
                         ? std::clamp(self.currentHp / self.maxHp.getFinal(), 0.0, 1.0)
                         : 0.0;
    QColor hpC = hpRatio > 0.5    ? QColor(80, 180, 255)
                 : hpRatio > 0.25 ? QColor(255, 200, 60)
                                  : QColor(255, 70, 70);
    r.queueRect(tx, ty - 12.0, ts * hpRatio, 6.0, hpC, 7);

    double gpa = hpRatio * 4.0;
    r.queueText(QString::number(gpa, 'f', 1),
                tx + ts / 2.0 - 10.0, ty + ts / 2.0 - 10.0,
                Qt::black, 8);
}

// ========== Paint 渲染 ==========
void TowerBehavior::renderSelf(const ChessInstance &self, QPainter &p,
                               double radius)
{
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(QColor(80, 160, 255), 3.0));
    p.setBrush(QColor(40, 100, 220, 200));
    p.drawRect(QRectF(-35, -35, 70, 70));

    double hpRatio = self.maxHp.getFinal() > 0
                         ? std::clamp(self.currentHp / self.maxHp.getFinal(), 0.0, 1.0)
                         : 0.0;
    QColor hpC = hpRatio > 0.5    ? QColor(80, 180, 255)
                 : hpRatio > 0.25 ? QColor(255, 200, 60)
                                  : QColor(255, 70, 70);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(40, 40, 40, 200));
    p.drawRect(QRectF(-35, -47, 70, 6));
    p.setBrush(hpC);
    p.drawRect(QRectF(-35, -47, 70 * hpRatio, 6));

    QFont f("Microsoft YaHei", 14);
    f.setBold(true);
    p.setFont(f);
    p.setPen(Qt::black);
    p.drawText(QRectF(-35, -30, 70, 60), Qt::AlignCenter,
               QString::number(hpRatio * 4.0, 'f', 1));
}

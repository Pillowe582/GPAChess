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
                           { return e.isAlive && e.isRequired; });
    if (it == enemies.end())
        return;

    // 伤害 = 敌人血量上限 × (1 + (1 - HP%) × 2)
    double hpRatio = self.maxHp.getFinal() > 0
                         ? self.currentHp / self.maxHp.getFinal()
                         : 1.0;

    int dmg = static_cast<int>(it->maxHp.getFinal() * (0.01 + (1.0 - hpRatio) * 2.0));

    it->dealDamage(dmg, self, DamageType{DamageType::True, QColor("#46a6ff")});

    if (!it->isAlive)
    {
        if (rng->bounded(10) == 0)
            pendingGold += it->baseGoldReward;
        pendingExp += it->baseExpReward;
    }

    m_attackCooldown = 1.5;
}

// ========== Queue 渲染 ==========
void TowerBehavior::renderSelf(const ChessInstance &self, Renderer &renderer,
                               double x, double y)
{
    const double tx = 10.0, ts = 128.0, ty = 400.0 - ts / 2.0;
    QString texturePath = ":/texture/entity/tower.png";
    renderer.queueImage(texturePath, tx + ts / 2.0, ty + ts / 2.0, 0.0, 1.0, Qt::AlignCenter, 10);

    double hpRatio = self.maxHp.getFinal() > 0
                         ? std::clamp(self.currentHp / self.maxHp.getFinal(), 0.0, 1.0)
                         : 0.0;
    QColor hpC = hpRatio > 0.5    ? QColor(80, 180, 255)
                 : hpRatio > 0.25 ? QColor(255, 200, 60)
                                  : QColor(255, 70, 70);
    renderer.queueRect(tx, ty - 12.0, ts * hpRatio, 6.0, hpC, 7);

    double gpa = hpRatio * 4.0;
    renderer.queueText(QString::number(gpa, 'f', 1),
                       tx + ts / 2.0 - 10.0, ty + ts / 2.0 - 10.0,
                       Qt::black, 100);
}

// ========== Paint 渲染 ==========
void TowerBehavior::renderSelf(const ChessInstance &self, QPainter &p,
                               double radius)
{
    return; // 塔不使用 Paint 渲染
}

// ========== 回合开始 ==========
void TowerBehavior::onStart(ChessInstance &self)
{
    // 战斗开始时设置初始攻击冷却，避免塔立即开火
    m_attackCooldown = 1.5; // 塔的攻击间隔为1.5秒
}

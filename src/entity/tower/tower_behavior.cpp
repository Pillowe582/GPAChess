#include "tower/tower_behavior.h"
#include "state.h"
#include "render/renderer.h"

#include <algorithm>
#include <QRandomGenerator>

/// 默认防御塔：攻击第一个存活敌人，伤害随血量下降而上升
class DefaultTower : public TowerBehavior
{
public:
    void onStart() override {}

    // ========== 逻辑 tick ==========
    void tick(double dt,
              int &currentHp,
              int maxHp,
              double &cooldown,
              std::vector<EnemyInstance> &enemies,
              Renderer &renderer,
              int &pendingGold,
              int &pendingExp) override
    {
        auto rng = QRandomGenerator::global();
        auto jitter = [rng]() -> double
        { return (rng->generateDouble() - 0.5) * 30.0; };

        // ====== 攻击逻辑 ======
        // 冷却递减
        cooldown = std::max(0.0, cooldown - dt);
        if (cooldown > 0.0)
            return;

        // 找第一个存活敌人
        auto it = std::find_if(enemies.begin(), enemies.end(),
                               [](const EnemyInstance &e)
                               { return e.isAlive; });
        if (it == enemies.end())
            return;

        // 伤害 = 30 × (1 + (1 - HP%) × 3)
        double hpRatio = maxHp > 0
                             ? static_cast<double>(currentHp) / static_cast<double>(maxHp)
                             : 1.0;
        int dmg = static_cast<int>(30.0 * (1.0 + (1.0 - hpRatio) * 3.0));

        it->takeDamage(dmg);
        renderer.queueSplash(QString("-%1").arg(dmg),
                             it->posX + jitter(),
                             it->posY - 20 + jitter(),
                             QColor("#4696FF"));

        if (!it->isAlive)
        {
            if (rng->bounded(10) == 0)
                pendingGold += it->baseGoldReward;
            pendingExp += it->baseExpReward;
        }

        cooldown = 1.5;
    }
};

// ========== 塔渲染共享实现 ==========
void TowerBehavior::renderSelf(int currentHp, int maxHp, Renderer &r)
{
    const double tx = 10.0, ts = 70.0, ty = 400.0 - ts / 2.0;
    r.queueRect(tx, ty, ts, ts, QColor(40, 100, 220, 200), 5);

    r.queueRect(tx, ty - 12.0, ts, 6.0, QColor(40, 40, 40, 200), 6);

    double hpRatio = maxHp > 0 ? std::clamp(static_cast<double>(currentHp) / maxHp, 0.0, 1.0) : 0.0;
    QColor hpC = hpRatio > 0.5    ? QColor(80, 180, 255)
                 : hpRatio > 0.25 ? QColor(255, 200, 60)
                                  : QColor(255, 70, 70);
    r.queueRect(tx, ty - 12.0, ts * hpRatio, 6.0, hpC, 7);

    double gpa = hpRatio * 4.0;
    r.queueText(QString::number(gpa, 'f', 1),
                tx + ts / 2.0 - 10.0, ty + ts / 2.0 - 10.0,
                Qt::black, 8);
}

TowerBehavior *createTowerBehavior()
{
    return new DefaultTower();
}

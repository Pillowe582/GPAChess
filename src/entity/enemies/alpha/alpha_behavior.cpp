#include "alpha_behavior.h"
#include "state.h"
#include "render/renderer.h"
#include <cmath>
#include <QRandomGenerator>

void AlphaEnemy::tick(double dt, EnemyInstance &self,
                      std::vector<ChessInstance> &allies,
                      Renderer &renderer,
                      int &towerHp, int &pendingGold, int &pendingExp)
{
    auto rng = QRandomGenerator::global();
    auto jitter = [rng]() -> double
    { return (rng->generateDouble() - 0.5) * 30.0; };

    // ====== 近战逻辑 ======
    m_cooldown = std::max(0.0, m_cooldown - dt);

    // ---- 武器挥砍动画中 ----
    if (m_weapon.active)
    {
        m_weapon.elapsed += dt;
        m_weapon.angle += (2.0 * 3.14159265) * dt / 0.2;

        int currentRot = static_cast<int>(m_weapon.angle / (2.0 * 3.14159265));
        if (currentRot > m_weapon.rotationsDone)
        {
            m_weapon.rotationsDone = currentRot;
            for (auto &ally : allies)
            {
                if (!ally.isAlive || !ally.deployed)
                    continue;
                double dx = ally.posX - self.posX;
                double dy = ally.posY - self.posY;
                if (std::sqrt(dx * dx + dy * dy) < 120.0)
                {
                    int dmg = self.atk.getFinal();
                    ally.takeDamage(dmg);
                    renderer.queueSplash(QString("-%1").arg(dmg),
                                         ally.posX + jitter(),
                                         ally.posY - 20 + jitter(),
                                         QColor("#FFFFFF"));
                    break;
                }
            }
        }

        if (m_weapon.rotationsDone >= 3)
        {
            m_weapon.active = false;
            int spd = self.attackSpeed.getFinal();
            m_cooldown = spd > 0 ? (1.0 / spd) : 1.0;
        }

        // 渲染：旋转的钻石剑图片
        double rotationDeg = m_weapon.angle * 180.0 / 3.14159265;
        renderer.queueImage(":/texture/projectile/diamond_sword.png", 
                           self.posX, self.posY, 
                           rotationDeg, 2.0, Qt::AlignCenter, 90);
        return;
    }

    if (m_cooldown > 0.0)
        return;

    // ---- 找最近我方单位 ----
    ChessInstance *target = nullptr;
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
            target = &a;
        }
    }
    if (!target)
        return;

    double dx = target->posX - self.posX;
    double dy = target->posY - self.posY;
    double dist = std::sqrt(dx * dx + dy * dy);
    if (dist > 70.0)
    {
        double spd = self.speed * 100.0 * dt;
        if (spd > dist - 70.0)
            spd = dist - 70.0;
        self.posX += dx / dist * spd;
        self.posY += dy / dist * spd;
    }
    else
    {
        m_weapon.active = true;
        m_weapon.angle = 0.0;
        m_weapon.elapsed = 0.0;
        m_weapon.rotationsDone = 0;
        m_weapon.targetUuid = target->uuid;
    }
}

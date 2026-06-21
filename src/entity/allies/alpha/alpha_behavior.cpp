#include "alpha_behavior.h"
#include "state.h"
#include "renderer.h"
#include <cmath>
#include <QRandomGenerator>

void AlphaAlly::tick(double dt, ChessInstance &self,
                     std::vector<EnemyInstance> &enemies,
                     Renderer &renderer,
                     int &pendingGold, int &pendingExp)
{
    auto rng = QRandomGenerator::global();
    auto jitter = [rng]() -> double
    { return (rng->generateDouble() - 0.5) * 30.0; };

    m_cooldown = std::max(0.0, m_cooldown - dt);

    // ---- 武器挥砍动画中 ----
    if (m_weapon.active)
    {
        m_weapon.elapsed += dt;
        m_weapon.angle += (2.0 * 3.14159265) * dt / 0.2; // 0.2s 一圈

        int currentRot = static_cast<int>(m_weapon.angle / (2.0 * 3.14159265));
        if (currentRot > m_weapon.rotationsDone)
        {
            m_weapon.rotationsDone = currentRot;
            // 每圈造成一次范围伤害
            for (auto &enemy : enemies)
            {
                if (!enemy.isAlive)
                    continue;
                double dx = enemy.transform.x - self.transform.x;
                double dy = enemy.transform.y - self.transform.y;
                if (dx * dx + dy * dy < 14400.0)
                {
                    int dmg = self.atk.getFinal();
                    enemy.dealDamage(dmg, DamageType{DamageType::Physical, QColor("#FF8C32")});
                    if (!enemy.isAlive)
                    {
                        if (rng->bounded(10) == 0)
                            pendingGold += enemy.baseGoldReward;
                        pendingExp += enemy.baseExpReward;
                    }
                    break;
                }
            }
        }

        if (m_weapon.rotationsDone >= 3)
        {
            m_weapon.active = false;
            int spd = self.baseAttackSpeed;
            m_cooldown = spd > 0 ? (1.0 / spd) : 1.0;
        }

        // 渲染：旋转的钻石剑图片
        double rotationDeg = m_weapon.angle * 180.0 / 3.14159265;
        renderer.queueImage(":/texture/projectile/diamond_sword.png",
                            self.transform.x, self.transform.y,
                            rotationDeg, 1.0, Qt::AlignCenter, 90);
        return;
    }

    // ---- 冷却中，不行动 ----
    if (m_cooldown > 0.0)
        return;

    // ---- 找最近敌人 ----
    LivingEntity *target = nullptr;
    double bestDist = 1e18;
    for (auto &e : enemies)
    {
        if (!e.isAlive)
            continue;
        double dx = e.transform.x - self.transform.x;
        double dy = e.transform.y - self.transform.y;
        double d = std::sqrt(dx * dx + dy * dy);
        if (d < bestDist)
        {
            bestDist = d;
            target = &e;
        }
    }
    if (!target)
        return;

    // ---- 移动向目标（停在70px处）----
    double dx = target->transform.x - self.transform.x;
    double dy = target->transform.y - self.transform.y;
    double dist = std::sqrt(dx * dx + dy * dy);
    if (dist > 70.0)
    {
        double spd = self.transform.scaleX * 100.0 * dt;
        if (spd > dist - 70.0)
            spd = dist - 70.0;
        self.transform.x += dx / dist * spd;
        self.transform.y += dy / dist * spd;
    }
    else
    {
        // 进入近战范围，激活武器
        m_weapon.active = true;
        m_weapon.angle = 0.0;
        m_weapon.elapsed = 0.0;
        m_weapon.rotationsDone = 0;
        m_weapon.targetUuid = target->getUuid();
    }
}

#include "alpha_behavior.h"
#include "state.h"
#include <cmath>
#include <QRandomGenerator>

void AlphaAlly::tick(double dt, ChessInstance &self,
                     std::vector<EnemyInstance> &enemies,
                     std::vector<DrawCmd> &draws,
                     int &pendingGold, int &pendingExp,
                     const SplashFn &splash)
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
                double dx = enemy.posX - self.posX;
                double dy = enemy.posY - self.posY;
                if (std::sqrt(dx * dx + dy * dy) < 120.0)
                {
                    int dmg = self.atk.getFinal();
                    enemy.takeDamage(dmg);
                    splash(QString("-%1").arg(dmg),
                           enemy.posX + jitter(),
                           enemy.posY - 20 + jitter(),
                           QStringLiteral("#FF8C32"));
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
            int spd = self.attackSpeed.getFinal();
            m_cooldown = spd > 0 ? (1.0 / spd) : 1.0;
        }

        // 渲染：旋转矩形
        DrawCmd cmd;
        cmd.kind = DrawCmd::RotatedRect;
        cmd.x = self.posX;
        cmd.y = self.posY;
        cmd.param1 = 55.0; // 半宽
        cmd.param2 = 8.0;  // 半高
        cmd.angle = m_weapon.angle;
        cmd.color = QColor(255, 140, 50, 200);
        cmd.zValue = 90;
        draws.push_back(cmd);
        return;
    }

    // ---- 冷却中，不行动 ----
    if (m_cooldown > 0.0)
        return;

    // ---- 找最近敌人 ----
    EnemyInstance *target = nullptr;
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
            target = &e;
        }
    }
    if (!target)
        return;

    // ---- 移动向目标（停在70px处）----
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
        // 进入近战范围，激活武器
        m_weapon.active = true;
        m_weapon.angle = 0.0;
        m_weapon.elapsed = 0.0;
        m_weapon.rotationsDone = 0;
        m_weapon.targetUuid = target->uuid;
    }
}

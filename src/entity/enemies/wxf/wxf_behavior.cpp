#include "wxf_behavior.h"
#include "state.h"
#include "renderer.h"
#include <cmath>
#include <QRandomGenerator>

void WXFEnemy::tick(double dt, EnemyInstance &self,
                    std::vector<std::unique_ptr<ChessInstance>> &allies,
                    Renderer &renderer,
                    int &pendingGold, int &pendingExp)
{

    // ====== 近战逻辑 ======
    m_cooldown = std::max(0.0, m_cooldown - dt);

    // ---- 武器挥砍动画中 ----
    if (m_weapon.active)
    {

        m_weapon.elapsed += dt;
        m_weapon.angle += (m_weapon.rotSpeed * 3.14159265) * dt / 0.2;
        m_weapon.rotSpeed += m_weapon.rotSpeed * dt; // 加速旋转

        int currentRot = -1 * static_cast<int>(m_weapon.angle / (2.0 * 3.14159265));
        if (currentRot > m_weapon.rotationsDone)
        {
            m_weapon.rotationsDone = currentRot;
            for (auto &ally : allies)
            {
                if (!ally->isAlive || !ally->deployed)
                    continue;
                double dx = ally->transform.x - self.transform.x;
                double dy = ally->transform.y - self.transform.y;
                if (std::sqrt(dx * dx + dy * dy) < 200.0)
                {
                    int dmg = self.atk.getFinal();
                    ally->dealDamage(dmg, DamageType{DamageType::Physical, QColor("#00ffc3")});

                    break;
                }
            }
        }

        if (m_weapon.rotationsDone >= 5)
        {
            m_weapon.rotSpeed = -0.5f; // 重置旋转速度
            m_weapon.active = false;
            int spd = self.baseAttackSpeed;
            m_cooldown = spd > 0 ? (1.0 / spd) : 1.0;
        }

        // 渲染：旋转的钻石剑图片
        double rotationDeg = m_weapon.angle * 180.0 / 3.14159265;
        renderer.queueImage(":/texture/projectile/oint.png",
                            self.transform.x + 50 * m_targetVec.normalized().x(), self.transform.y + 50 * m_targetVec.normalized().y(),
                            rotationDeg, 2.0, Qt::AlignCenter, 90);
        return;
    }

    if (m_cooldown > 0.0)
        return;

    // ---- 找最近我方单位（排除塔）----
    ChessInstance *target = nullptr;
    double bestDist = 1e18;
    for (auto &ally : allies)
    {
        // 跳过塔，优先攻击普通单位
        if (!ally->isAlive || !ally->deployed || ally->isTower)
            continue;
        double dx = ally->transform.x - self.transform.x;
        double dy = ally->transform.y - self.transform.y;
        double d = dx * dx + dy * dy;
        if (d < bestDist)
        {
            bestDist = d;
            target = ally.get();
            m_targetVec = QVector2D(dx, dy);
        }
    }

    // 如果所有非塔单位都阵亡，则攻击塔
    if (!target)
    {
        for (auto &ally : allies)
        {
            if (!ally->isAlive || !ally->deployed || !ally->isTower)
                continue;
            double dx = ally->transform.x - self.transform.x;
            double dy = ally->transform.y - self.transform.y;
            double d = dx * dx + dy * dy;
            if (d < bestDist)
            {
                bestDist = d;
                target = ally.get();
                m_targetVec = QVector2D(dx, dy);
            }
        }
    }

    if (!target)
        return;

    double dx = target->transform.x - self.transform.x;
    double dy = target->transform.y - self.transform.y;
    double dist = std::sqrt(dx * dx + dy * dy);
    if (dist > 70.0)
    {
        double spd = self.speed * 100.0 * dt;
        if (spd > dist - 70.0)
            spd = dist - 70.0;
        self.transform.x += dx / dist * spd;
        self.transform.y += dy / dist * spd;
    }
    else if (dist < 50.0)
    {
        double spd = -self.speed * 100.0 * dt;
        self.transform.x -= dx / dist * spd;
        self.transform.y -= dy / dist * spd;
    }

    if (dist < 100.0)
    {
        m_weapon.active = true;
        m_weapon.angle = 0.0;
        m_weapon.elapsed = 0.0;
        m_weapon.rotationsDone = 0;
        m_weapon.targetUuid = target->getUuid();
    }
}

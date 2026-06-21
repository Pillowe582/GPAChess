#include "beta_behavior.h"
#include "state.h"
#include "renderer.h"
#include <cmath>
#include <QRandomGenerator>

void BetaEnemy::tick(double dt, EnemyInstance &self,
                     std::vector<std::unique_ptr<ChessInstance>> &allies,
                     Renderer &renderer,
                     int &pendingGold, int &pendingExp)
{

    // ====== 远程逻辑 ======
    // ====== 推进已有子弹 ======
    for (size_t i = 0; i < m_bullets.size();)
    {
        auto &bullet = m_bullets[i];
        bullet.x += bullet.vx * dt;
        bullet.y += bullet.vy * dt;

        // 使用子弹图片代替圆形
        renderer.queueImage(":/texture/projectile/bullet.png",
                            bullet.x, bullet.y,
                            0.0, 1.0, Qt::AlignCenter, 100);

        bool hit = false;
        for (auto &ally : allies)
        {
            if (!ally->isAlive || !ally->deployed)
                continue;
            double dx = bullet.x - ally->transform.x;
            double dy = bullet.y - ally->transform.y;
            if (std::sqrt(dx * dx + dy * dy) < 40.0)
            {
                ally->dealDamage(bullet.damage, self,
                                 DamageType{DamageType::Physical, QColor("#ffffff")});
                hit = true;
                break;
            }
        }
        if (hit || bullet.x < -100 || bullet.x > 2020 || bullet.y < -100 || bullet.y > 900)
            m_bullets.erase(m_bullets.begin() + static_cast<long long>(i));
        else
            ++i;
    }

    // ====== 冷却 ======
    m_cooldown = std::max(0.0, m_cooldown - dt);
    if (m_cooldown > 0.0)
        return;

    // ====== 找最远我方单位======
    ChessInstance *target = nullptr;
    double bestDist = -1.0;
    for (auto &a : allies)
    {
        // 跳过塔，优先攻击普通单位
        if (!a->isAlive || !a->deployed || a->isTower)
            continue;
        double dx = a->transform.x - self.transform.x;
        double dy = a->transform.y - self.transform.y;
        double d = std::sqrt(dx * dx + dy * dy);
        if (d > bestDist)
        {
            bestDist = d;
            target = a.get();
        }
    }

    // 如果所有非塔单位都阵亡，则攻击塔
    if (!target)
    {
        for (auto &a : allies)
        {
            if (!a->isAlive || !a->deployed || !a->isTower)
                continue;
            double dx = a->transform.x - self.transform.x;
            double dy = a->transform.y - self.transform.y;
            double d = std::sqrt(dx * dx + dy * dy);
            if (d > bestDist)
            {
                bestDist = d;
                target = a.get();
            }
        }
    }

    int atkSpd = self.baseAttackSpeed;
    double interval = atkSpd > 0 ? (1.0 / atkSpd) : 1.0;

    if (!target)
    {
        m_cooldown = interval;
        return;
    }

    // ====== 发射子弹 ======
    Bullet b;
    b.x = self.transform.x;
    b.y = self.transform.y;
    double tdx = target->transform.x - self.transform.x;
    double tdy = target->transform.y - self.transform.y;
    double td = std::sqrt(tdx * tdx + tdy * tdy);
    double spd = 400.0;
    b.vx = tdx / td * spd;
    b.vy = tdy / td * spd;
    b.damage = self.atk.getFinal();
    m_bullets.push_back(b);
    m_cooldown = interval;
}

void BetaEnemy::onStart(EnemyInstance &self)
{
    // 战斗开始时设置初始冷却时间，避免立即发射子弹
    int atkSpd = self.baseAttackSpeed;
    double interval = atkSpd > 0 ? (1.0 / atkSpd) : 1.0;
    m_cooldown = interval;
}

#include "beta_behavior.h"
#include "state.h"
#include "render/renderer.h"
#include <cmath>
#include <QRandomGenerator>

void BetaEnemy::tick(double dt, EnemyInstance &self,
                     std::vector<ChessInstance> &allies,
                     Renderer &renderer,
                     int &towerHp, int &pendingGold, int &pendingExp)
{
    auto rng = QRandomGenerator::global();
    auto jitter = [rng]() -> double
    { return (rng->generateDouble() - 0.5) * 30.0; };

    // ====== 远程逻辑 ======
    // ====== 推进已有子弹 ======
    for (size_t i = 0; i < m_bullets.size();)
    {
        auto &b = m_bullets[i];
        b.x += b.vx * dt;
        b.y += b.vy * dt;

        // 使用子弹图片代替圆形
        renderer.queueImage(":/texture/projectile/bullet.png", 
                           b.x, b.y, 
                           0.0, 1.0, Qt::AlignCenter, 100);

        bool hit = false;
        for (auto &ally : allies)
        {
            if (!ally.isAlive || !ally.deployed)
                continue;
            double dx = b.x - ally.posX;
            double dy = b.y - ally.posY;
            if (std::sqrt(dx * dx + dy * dy) < 40.0)
            {
                ally.takeDamage(b.damage);
                renderer.queueSplash(QString("-%1").arg(b.damage),
                                     ally.posX + jitter(),
                                     ally.posY - 20 + jitter(),
                                     QColor("#FFFFFF"));
                hit = true;
                break;
            }
        }
        if (hit || b.x < -100 || b.x > 2020 || b.y < -100 || b.y > 900)
            m_bullets.erase(m_bullets.begin() + static_cast<long long>(i));
        else
            ++i;
    }

    // ====== 冷却 ======
    m_cooldown = std::max(0.0, m_cooldown - dt);
    if (m_cooldown > 0.0)
        return;

    // ====== 找最远我方单位 ======
    ChessInstance *target = nullptr;
    double bestDist = -1.0;
    for (auto &a : allies)
    {
        if (!a.isAlive || !a.deployed)
            continue;
        double dx = a.posX - self.posX;
        double dy = a.posY - self.posY;
        double d = std::sqrt(dx * dx + dy * dy);
        if (d > bestDist)
        {
            bestDist = d;
            target = &a;
        }
    }

    int atkSpd = self.attackSpeed.getFinal();
    double interval = atkSpd > 0 ? (1.0 / atkSpd) : 1.0;

    if (!target)
    {
        // 没有我方单位 → 攻击塔
        int dmg = self.atk.getFinal();
        towerHp -= dmg;
        renderer.queueSplash(QString("-%1").arg(dmg), 80.0 + jitter(), 400.0 + jitter(), QColor("#FFFFFF"));
        m_cooldown = interval;
        return;
    }

    // ====== 发射子弹 ======
    Bullet b;
    b.x = self.posX;
    b.y = self.posY;
    double tdx = target->posX - self.posX;
    double tdy = target->posY - self.posY;
    double td = std::sqrt(tdx * tdx + tdy * tdy);
    double spd = 400.0;
    b.vx = tdx / td * spd;
    b.vy = tdy / td * spd;
    b.damage = self.atk.getFinal();
    m_bullets.push_back(b);
    m_cooldown = interval;
}

#include "beta_behavior.h"
#include "state.h"
#include "render/renderer.h"
#include <cmath>
#include <QRandomGenerator>

void BetaAlly::tick(double dt, ChessInstance &self,
                    std::vector<EnemyInstance> &enemies,
                    Renderer &renderer,
                    int &pendingGold, int &pendingExp)
{
    auto rng = QRandomGenerator::global();
    auto jitter = [rng]() -> double
    { return (rng->generateDouble() - 0.5) * 30.0; };

    // ====== 推进已有子弹 ======
    for (size_t i = 0; i < m_bullets.size();)
    {
        auto &b = m_bullets[i];
        b.x += b.vx * dt;
        b.y += b.vy * dt;

        renderer.queueCircle(b.x, b.y, 6.0,
                             QColor(100, 220, 80, 220), 100);

        bool hit = false;
        for (auto &enemy : enemies)
        {
            if (!enemy.isAlive)
                continue;
            double dx = b.x - enemy.posX;
            double dy = b.y - enemy.posY;
            if (std::sqrt(dx * dx + dy * dy) < 40.0)
            {
                enemy.takeDamage(b.damage);
                renderer.queueSplash(QString("-%1").arg(b.damage),
                                     enemy.posX + jitter(),
                                     enemy.posY - 20 + jitter(),
                                     QColor("#64DC50"));
                if (!enemy.isAlive)
                {
                    if (rng->bounded(10) == 0)
                        pendingGold += enemy.baseGoldReward;
                    pendingExp += enemy.baseExpReward;
                }
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

    // ====== 找最远敌人 ======
    EnemyInstance *target = nullptr;
    double bestDist = -1.0;
    for (auto &e : enemies)
    {
        if (!e.isAlive)
            continue;
        double dx = e.posX - self.posX;
        double dy = e.posY - self.posY;
        double d = std::sqrt(dx * dx + dy * dy);
        if (d > bestDist)
        {
            bestDist = d;
            target = &e;
        }
    }
    if (!target)
        return;

    // ====== 发射子弹 ======
    Bullet b;
    b.x = self.posX;
    b.y = self.posY;
    double tdx = target->posX - self.posX;
    double tdy = target->posY - self.posY;
    double td = std::sqrt(tdx * tdx + tdy * tdy);
    double spd = 400.0; // 子弹速度 400px/s
    b.vx = tdx / td * spd;
    b.vy = tdy / td * spd;
    b.damage = self.atk.getFinal();
    m_bullets.push_back(b);

    int atkSpd = self.attackSpeed.getFinal();
    m_cooldown = atkSpd > 0 ? (1.0 / atkSpd) : 1.0;
}

#include "overwhelmed_bro_behavior.h"
#include "state.h"
#include "renderer.h"
#include "game_manager.h"
#include <cmath>
#include <QRandomGenerator>

void OverwhelmedBroAlly::tick(double dt, BaseEntity &baseSelf, GameManager &gameManager)
{
    AllyInstance &self = static_cast<AllyInstance &>(baseSelf);
    const auto &enemies = gameManager.getGameEntities().enemies;
    Renderer &renderer = gameManager.getRenderer();

    // 1. 推进已有子弹并检测碰撞
    updateBullets(dt, const_cast<std::vector<EnemyInstance *> &>(enemies), renderer, self, gameManager);

    // 2. 冷却递减
    m_currentCooldown = std::max(0.0, m_currentCooldown - dt);
    if (m_currentCooldown > 0.0)
        return;

    // 3. 寻找目标
    EnemyInstance *target = findFarthestEnemy(enemies, self);
    if (!target)
        return;

    // 4. 发射子弹
    fireBullet(self, target);
    m_currentCooldown = m_maxCooldown;
}

void OverwhelmedBroAlly::updateBullets(double dt,
                                       std::vector<EnemyInstance *> &enemies,
                                       Renderer &renderer,
                                       AllyInstance &self,
                                       GameManager &gameManager)
{
    auto rng = QRandomGenerator::global();

    for (size_t i = 0; i < m_bullets.size();)
    {
        auto &b = m_bullets[i];
        b.x += b.vx * dt;
        b.y += b.vy * dt;
        b.rot += 180.0 * dt; // 旋转效果

        // 使用子弹图片代替圆形
        renderer.queueImage(":/texture/projectile/tropical_tea.png",
                            b.x, b.y,
                            b.rot, 0.7, Qt::AlignCenter, 100);

        bool hit = false;
        for (auto &enemy : enemies)
        {
            if (!enemy->isAlive)
                continue;
            double dx = b.x - enemy->transform.x;
            double dy = b.y - enemy->transform.y;
            if (std::sqrt(dx * dx + dy * dy) < 100.0)
            {
                enemy->dealDamage(b.damage, self, DamageType{DamageType::Physical, QColor("#b93ec0")});
                if (!enemy->isAlive)
                {
                    int gold = (rng->bounded(10) == 0) ? enemy->baseGoldReward : 0;
                    gameManager.addPendingRewards(gold, enemy->baseExpReward);
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
}

EnemyInstance *OverwhelmedBroAlly::findFarthestEnemy(const std::vector<EnemyInstance *> &enemies,
                                                     const AllyInstance &self)
{
    EnemyInstance *target = nullptr;
    double bestDist = -1.0;
    for (auto &e : enemies)
    {
        if (!e->isAlive)
            continue;
        double dx = e->transform.x - self.transform.x;
        double dy = e->transform.y - self.transform.y;
        double d = std::sqrt(dx * dx + dy * dy);
        if (d > bestDist)
        {
            bestDist = d;
            target = e;
        }
    }
    return target;
}

void OverwhelmedBroAlly::fireBullet(const AllyInstance &self, EnemyInstance *target)
{
    Bullet b;
    b.x = self.transform.x;
    b.y = self.transform.y;
    double tdx = target->transform.x - self.transform.x;
    double tdy = target->transform.y - self.transform.y;
    double td = std::sqrt(tdx * tdx + tdy * tdy);
    double spd = 400.0; // 子弹速度 400px/s
    b.vx = tdx / td * spd;
    b.vy = tdy / td * spd;
    b.damage = self.atk.getFinal();
    m_bullets.push_back(b);
}

void OverwhelmedBroAlly::onStart(AllyInstance &self)
{
    // 战斗开始时清空上一回合残留的子弹
    m_bullets.clear();

    // 计算技能冷却时间
    int atkSpd = self.baseAttackSpeed;
    m_maxCooldown = atkSpd > 0 ? (2.0 / atkSpd) : 2.0;

    // 设置初始冷却时间，避免立即发射子弹
    m_currentCooldown = m_maxCooldown;
}
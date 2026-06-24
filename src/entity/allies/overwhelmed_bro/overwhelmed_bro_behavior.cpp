#include "overwhelmed_bro_behavior.h"
#include "state.h"
#include "renderer.h"
#include "game_manager.h"
#include <cmath>
#include <QRandomGenerator>
// %初始化

void OverwhelmedBroAlly::onStart(AllyInstance &self)
{
    // 战斗开始时清空上一回合残留的子弹
    m_bullets.clear();

    // 清空热带风味
    m_teaSpout.elapsedTime = -1;
    m_teaSpout.hitCount = 0;

    // 计算技能冷却时间
    int atkSpd = self.baseAttackSpeed;
    m_maxCooldown = atkSpd > 0 ? (2.0 / atkSpd) : 2.0;

    // 设置初始冷却时间，避免立即发射子弹
    m_currentCooldown = m_maxCooldown;
}
// % 每刻调用方法
void OverwhelmedBroAlly::tick(double dt, BaseEntity &baseSelf, GameManager &gameManager)
{
    AllyInstance &self = static_cast<AllyInstance &>(baseSelf);
    const auto &enemies = gameManager.getGameEntities().enemies;
    Renderer &renderer = gameManager.getRenderer();

    updatePos(dt, self, enemies);

    // 1. 推进已有子弹并检测碰撞
    updateBullets(dt, self, gameManager);
    if (m_teaSpout.elapsedTime >= 0)
    {
        updateTropicalTeaSpout(dt, self, gameManager);
    }

    // 2. 冷却递减
    m_currentCooldown = std::max(0.0, m_currentCooldown - dt);
    if (m_currentCooldown > 0.0)
        return;

    // 3. 决定技能
    if (self.currentMp >= self.maxMp)
    {
        // 喷射热带风味冰红茶
        EnemyInstance *target = findEnemy(enemies, self, 0);
        if (!target)
            return;
        double distance = std::sqrt(std::pow(target->transform.x - self.transform.x, 2) + std::pow(target->transform.y - self.transform.y, 2));
        if (distance > 300) // 目标不在范围内
            return;
        m_teaSpout.x = self.transform.x;
        m_teaSpout.y = self.transform.y;
        m_teaSpout.rot = 0.0;
        m_teaSpout.elapsedTime = 0.0;
        m_teaSpout.hitCount = 0;
        m_teaSpout.damage = static_cast<int>(self.atk.getFinal() * 0.5);

        self.currentMp -= self.maxMp;
    }
    else
    {
        // 发射普通子弹
        EnemyInstance *target = findEnemy(enemies, self, 114514);
        if (!target)
            return;
        fireBullet(self, target);
    }
    m_currentCooldown = m_maxCooldown;
}

// % 走位
void OverwhelmedBroAlly::updatePos(double dt, AllyInstance &self, const std::vector<EnemyInstance *> &enemies)
{
    if (m_teaSpout.elapsedTime >= 0)
        return;

    // 找到最近的敌人
    EnemyInstance *closestEnemy = findEnemy(enemies, self, 0);

    if (!closestEnemy)
        return;

    // 方向向量
    double dx = closestEnemy->transform.x - self.transform.x;
    double dy = closestEnemy->transform.y - self.transform.y;
    double dist = std::sqrt(dx * dx + dy * dy);

    if (dist < 0.001)
        return;

    double dirX = dx / dist; // 指向敌人的单位向量
    double dirY = dy / dist;

    double moveSpeed = 150.0; // 移动速度 px/s

    if (self.currentMp < self.maxMp) // MP未满：保持距离 400~600
    {
        if (dist < 400) // 太近，后退
        {
            self.transform.x -= dirX * moveSpeed * dt;
            self.transform.y -= dirY * moveSpeed * dt;
        }
        else if (dist > 600) // 太远，靠近
        {
            self.transform.x += dirX * moveSpeed * dt;
            self.transform.y += dirY * moveSpeed * dt;
        }
    }
    else // MP已满：保持距离 150~250
    {
        if (dist < 150) // 太近，后退
        {
            self.transform.x -= dirX * moveSpeed * dt;
            self.transform.y -= dirY * moveSpeed * dt;
        }
        else if (dist > 250) // 太远，靠近
        {
            self.transform.x += dirX * moveSpeed * dt;
            self.transform.y += dirY * moveSpeed * dt;
        }
    }
}

// % 更新子弹
void OverwhelmedBroAlly::updateBullets(double dt, AllyInstance &self, GameManager &gameManager)
{
    // 获取上下文
    auto rng = QRandomGenerator::global();
    Renderer &renderer = gameManager.getRenderer();
    std::vector<EnemyInstance *> &enemies = const_cast<std::vector<EnemyInstance *> &>(gameManager.getGameEntities().enemies);

    for (size_t i = 0; i < m_bullets.size();)
    {
        auto &b = m_bullets[i];
        b.x += b.vx * dt;
        b.y += b.vy * dt;
        b.rot += 180.0 * dt; // 旋转效果

        // 使用子弹图片代替圆形
        renderer.queueImage(":/texture/projectile/luxine.png",
                            b.x, b.y,
                            b.rot, 0.5, Qt::AlignCenter, 100);

        bool hit = false;
        for (auto &enemy : enemies)
        {
            if (!enemy->isAlive)
                continue;
            double dx = b.x - enemy->transform.x;
            double dy = b.y - enemy->transform.y;
            if (std::sqrt(dx * dx + dy * dy) < 50.0)
            {
                enemy->dealDamage(b.damage, self, DamageType{DamageType::Physical, QColor("#b93ec0")});
                if (!enemy->isAlive)
                {
                    int gold = (rng->bounded(10) == 0) ? enemy->baseGoldReward : 0;
                    gameManager.addPendingRewards(gold, enemy->baseExpReward);
                }
                self.currentMp += 50;
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

// % 索敌
EnemyInstance *OverwhelmedBroAlly::findEnemy(const std::vector<EnemyInstance *> &enemies,
                                             const AllyInstance &self, int bestDist)
{
    EnemyInstance *target = nullptr;

    // 与期望距离差值最小的敌人即为目标，此处先初始化为一个极大值
    double minDiff = std::numeric_limits<double>::max();
    for (auto &e : enemies)
    {
        if (!e->isAlive)
            continue;
        double dx = e->transform.x - self.transform.x;
        double dy = e->transform.y - self.transform.y;
        double d = std::abs(dx) + std::abs(dy); // 曼哈顿距离
        double diff = std::abs(d - bestDist);   // 与期望距离的差值
        if (diff < minDiff)
        {
            minDiff = diff;
            target = e;
            m_targetDirection = QVector2D(dx, dy).normalized();
        }
    }
    return target;
}

// % 发射子弹
void OverwhelmedBroAlly::fireBullet(const AllyInstance &self, EnemyInstance *target)
{
    LuxineBullet b;
    b.x = self.transform.x;
    b.y = self.transform.y;

    double spd = 400.0; // 子弹速度 400px/s
    b.vx = m_targetDirection.x() * spd;
    b.vy = m_targetDirection.y() * spd;

    b.damage = self.atk.getFinal();
    m_bullets.push_back(b);
}

// % 热带风味喷射战士
void OverwhelmedBroAlly::updateTropicalTeaSpout(double dt, AllyInstance &self, GameManager &gameManager)
{
    // 技能持续期间无法发射子弹
    m_currentCooldown = 1.0;

    double baseRotAngle = atan2(m_targetDirection.y(), m_targetDirection.x()) * 180.0 / M_PI;
    m_teaSpout.elapsedTime += dt;
    // 1. 蓄力一秒钟
    if (m_teaSpout.elapsedTime <= 1.0) // 第一秒
    {
        gameManager.getRenderer().queueImage(":/texture/projectile/tropical_tea.png",
                                             self.transform.x + 50 * m_targetDirection.x(),
                                             self.transform.y + 50 * m_targetDirection.y(),
                                             baseRotAngle + 10 * std::sin(8 * M_PI * m_teaSpout.elapsedTime), 0.5, Qt::AlignCenter, 100);
        return;
    }

    else if (m_teaSpout.elapsedTime > 1.0 && m_teaSpout.hitCount < 7) // 进入喷射战士状态
    {

        gameManager.getRenderer().queueImage(":/texture/projectile/tropical_tea_spout.png",
                                             self.transform.x + 50 * m_targetDirection.x(),
                                             self.transform.y + 50 * m_targetDirection.y(),
                                             baseRotAngle + 90 + 10 * std::sin(2 * M_PI * m_teaSpout.elapsedTime), 5.0, Qt::AlignCenter, 100);

        if (m_teaSpout.elapsedTime - 1 > 0.5 * m_teaSpout.hitCount) // 3秒7喷
        {
            ++m_teaSpout.hitCount;
            // 遍历所有敌人，对喷射范围内的敌人造成伤害
            for (auto &enemy : gameManager.getGameEntities().enemies)
            {
                if (!enemy->isAlive)
                    continue;
                double dx = enemy->transform.x - self.transform.x;
                double dy = enemy->transform.y - self.transform.y;
                double d = std::sqrt(dx * dx + dy * dy);
                double dtheta = std::atan2(dy, dx) - baseRotAngle * M_PI / 180.0;
                if (d < 400.0 && std::abs(dtheta) < M_PI / 12) // 范围：距离400且角度在±15度内
                {
                    enemy->dealDamage(m_teaSpout.damage, self, DamageType{DamageType::Physical, QColor("#b93ec0")});
                    if (!enemy->isAlive)
                    {
                        int gold = (QRandomGenerator::global()->bounded(10) == 0) ? enemy->baseGoldReward : 0;
                        gameManager.addPendingRewards(gold, enemy->baseExpReward);
                    }
                }
            }
        }
    }
    else
    {
        m_teaSpout.elapsedTime = -1;
        m_teaSpout.hitCount = 0;
    }
}
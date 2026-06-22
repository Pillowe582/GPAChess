#include "calculus_behavior.h"
#include "state.h"
#include "renderer.h"
#include "game_manager.h"
#include <cmath>
#include <QRandomGenerator>
#include <QPen>
#include <algorithm>
// % 初始化
void CalculusEnemy::onStart(EnemyInstance &self)
{
    m_weapon.reset();
    m_differential.reset();
    m_differentialPen = QPen(QColor("#a4f9e7"), 4.0);
    int spd = self.baseAttackSpeed;
    m_cooldown = spd > 0 ? (1.0 / spd) : 1.0;
}

// % 每刻逻辑
void CalculusEnemy::tick(double dt, BaseEntity &baseSelf, GameManager &gameManager)
{
    EnemyInstance &self = static_cast<EnemyInstance &>(baseSelf);
    const auto &allies = gameManager.getPlayerAssets().ownedChesses;
    Renderer &renderer = gameManager.getRenderer();
    AllyInstance *target = findClosestAlly(allies, self);
    if (!target)
        return;

    updatePos(dt, self, target);

    // ====== 近战逻辑 ======
    m_cooldown = std::max(0.0, m_cooldown - dt);

    // 1. 武器挥砍动画（优先）
    if (m_weapon.active)
    {
        updateInteger(dt, allies, renderer, self);
        return;
    }

    // 2. 大招发动期间
    if (m_differential.active)
    {
        updateDifferential(dt, allies, renderer, self);
        return;
    }

    if (m_cooldown > 0.0)
        return;

    // 3. MP 满 → 发动大招
    if (self.currentMp >= self.maxMp)
    {
        m_differentialPen.setColor(QColor("#a4f9e7"));
        m_differential.reset();
        findInfSupAlly(allies, self, m_differential.infX, m_differential.supX);
        m_differential.active = true;
        m_differential.damage = self.atk.getFinal() * 3;
        self.currentMp -= self.maxMp;
        return;
    }

    // 5. 近距离触发攻击
    double dx = target->transform.x - self.transform.x;
    double dy = target->transform.y - self.transform.y;
    double dist = std::sqrt(dx * dx + dy * dy);
    if (dist < 100.0)
    {
        m_weapon.active = true;
        m_weapon.angle = 0.0;
        m_weapon.elapsed = 0.0;
        m_weapon.rotationsDone = 0;
        m_targetVec = QVector2D(dx, dy);
        m_weapon.targetUuid = target->getUuid();
    }
}
// % 普攻每帧逻辑
void CalculusEnemy::updateInteger(double dt,
                                  const std::vector<std::unique_ptr<AllyInstance>> &allies,
                                  Renderer &renderer,
                                  EnemyInstance &self)
{
    m_weapon.elapsed += dt;
    m_weapon.angle += (m_weapon.rotSpeed * 3.14159265) * dt / 0.2;
    m_weapon.rotSpeed += 1.5 * m_weapon.rotSpeed * dt;

    int currentRot = -1 * static_cast<int>(m_weapon.angle / (2.0 * 3.14159265));
    if (currentRot > m_weapon.rotationsDone)
    {
        self.currentMp += 10;
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
                ally->dealDamage(dmg, self, DamageType{DamageType::Physical, QColor("#00ffc3")});
            }
        }
    }

    if (m_weapon.rotationsDone >= 5)
    {
        m_weapon.rotSpeed = -0.5f;
        m_weapon.active = false;
        int spd = self.baseAttackSpeed;
        m_cooldown = spd > 0 ? (1.0 / spd) : 1.0;
    }

    double rotationDeg = m_weapon.angle * 180.0 / 3.14159265;
    renderer.queueImage(":/texture/projectile/oint.png",
                        self.transform.x + 50 * m_targetVec.normalized().x(), self.transform.y + 50 * m_targetVec.normalized().y(),
                        rotationDeg, 2.0, Qt::AlignCenter, 90);
}

// % 索敌
AllyInstance *CalculusEnemy::findClosestAlly(const std::vector<std::unique_ptr<AllyInstance>> &allies,
                                             const EnemyInstance &self)
{
    AllyInstance *target = nullptr;
    double bestDist = 1e18;
    for (auto &ally : allies)
    {
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

    if (!target && self.isRequired)
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
    return target;
}

void CalculusEnemy::findInfSupAlly(const std::vector<std::unique_ptr<AllyInstance>> &allies,
                                   const EnemyInstance &self,
                                   double &infX, double &supX)
{
    infX = 1e18;
    supX = -1e18;
    for (auto &ally : allies)
    {
        if (!ally->isAlive || !ally->deployed || ally->isTower)
            continue;
        double x = ally->transform.x;
        if (x < infX)
            infX = x;
        if (x > supX)
            supX = x;
    }
    // 如果没有存活单位，默认两边
    if (infX > supX)
    {
        infX = self.transform.x - 200;
        supX = self.transform.x + 200;
    }
}

// % 位移
void CalculusEnemy::updatePos(double dt, EnemyInstance &self, AllyInstance *target)
{
    double dx = target->transform.x - self.transform.x;
    double dy = target->transform.y - self.transform.y;
    double dist = std::sqrt(dx * dx + dy * dy);
    double spd = self.speed * 100.0 * dt;

    if (dist > 90.0)
    {

        if (spd > dist - 90.0)
            spd = dist - 90.0;
        self.transform.x += dx / dist * spd;
        self.transform.y += dy / dist * spd;
    }
    else if (dist < 70.0)
    {
        if (spd > 70.0 - dist)
            spd = 70.0 - dist;
        self.transform.x -= dx / dist * spd;
        self.transform.y -= dy / dist * spd;
    }
}

// % 大招每帧逻辑
void CalculusEnemy::updateDifferential(double dt, const std::vector<std::unique_ptr<AllyInstance>> &allies,
                                       Renderer &renderer, EnemyInstance &self)
{
    m_differential.elapsedTime += dt;
    m_differential.groupElapsed += dt;
    m_differential.lineSpawnTimer += dt;

    double groupInterval = 0.3;
    double lineIntervalSec = 0.0005; // 0.5ms
    double lineDuration = 0.3;
    int maxRounds = 4; // 最多4轮

    // 清理过期线条
    double now = m_differential.elapsedTime;
    auto &active = m_differential.activeLines;
    active.erase(std::remove_if(active.begin(), active.end(),
                                [now, lineDuration](const Differential::TempLine &ln)
                                {
                                    return (now - ln.bornTime) > lineDuration;
                                }),
                 active.end());

    // 开始新一轮
    if (m_differential.groupElapsed >= groupInterval && m_differential.roundCount < maxRounds)
    {
        m_differential.groupElapsed -= groupInterval;
        m_differential.roundCount++;

        // 2^n 个区域 → 2^n + 1 条线
        int totalLines = static_cast<int>(std::pow(2, m_differential.roundCount)) + 1;
        m_differential.currentGroupLineCount = totalLines;
        m_differential.linesSpawnedInGroup = 0;

        // 计算区域宽度（用于伤害判定）
        double dx = (m_differential.supX - m_differential.infX) / (totalLines - 1);
        m_differential.sectionWidth = dx;
    }

    // 生成线条
    if (m_differential.linesSpawnedInGroup < m_differential.currentGroupLineCount)
    {
        if (m_differential.lineSpawnTimer >= lineIntervalSec)
        {
            m_differential.lineSpawnTimer -= lineIntervalSec;

            int totalLines = m_differential.currentGroupLineCount;
            double dx = m_differential.sectionWidth;
            int i = m_differential.linesSpawnedInGroup;
            double linePos = m_differential.infX + dx * i;

            // 记录线条
            m_differential.activeLines.push_back({linePos, now});

            // 对碰到线的目标造成伤害
            for (auto &target : allies)
            {
                if (!target->isAlive || !target->deployed)
                    continue;
                double x = target->transform.x;
                if (std::abs(x - linePos) < 32) // 区域半宽
                {
                    int dmg = m_differential.damage / (totalLines - 1);
                    target->dealDamage(dmg, self, DamageType{DamageType::Physical, QColor("#00ffc8")});
                }
            }

            m_differential.linesSpawnedInGroup++;
        }
    }

    // 渲染活跃线条
    for (const auto &ln : active)
    {
        // 可选：淡出效果
        double age = now - ln.bornTime;
        double alpha = 1.0 - (age / lineDuration);
        QColor color = m_differentialPen.color();
        color.setAlpha(static_cast<int>(255 * alpha));
        QPen pen(color, m_differentialPen.width());

        renderer.queueLine(ln.x, 0, ln.x, 1080, pen, 90);
    }

    // 结束条件：完成4轮且所有线条消失
    if (m_differential.roundCount >= maxRounds && m_differential.linesSpawnedInGroup >= m_differential.currentGroupLineCount && active.empty())
    {
        m_differential.active = false;
        m_differential.elapsedTime = -1.0;
        m_differential.roundCount = 0;
        m_differential.activeLines.clear();
        int spd = self.baseAttackSpeed;
        m_cooldown = spd > 0 ? (1.0 / spd) : 1.0;
    }
}

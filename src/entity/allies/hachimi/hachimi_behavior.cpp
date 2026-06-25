#include "hachimi_behavior.h"
#include "state.h"
#include "print.h"
#include "renderer.h"
#include "game_manager.h"
#include <cmath>
#include <QRandomGenerator>

// % 初始化
void HachimiAlly::onStart(AllyInstance &self)
{
    // 重置哈基米状态
    m_state.reset();
    m_lastRecordedHp = self.getCurrentHp();
    m_lastNotHurtTimer = 0.0;
    m_target = nullptr;

    // 设置初始冷却时间，避免立即发动攻击
    int spd = self.baseAttackSpeed;
    m_cooldown = spd > 0 ? (1.0 / spd) : 1.0;
}
// % 状态切换
void HachimiAlly::switchState(HachimiState::State newState)
{
    m_state.state = newState;

    if (newState == HachimiState::O)
    {
        m_state.m_oStateTimer = 0.0; // O状态计时器重置
        m_state.attackedRounds = 0;  // 攻击次数重置
        m_cooldown = 0.5;            // 哈气状态冷却0.5秒
    }
}

// % 每刻逻辑
void HachimiAlly::tick(double dt, BaseEntity &baseSelf, GameManager &gameManager)
{
    m_gameManager = &gameManager; // 保存游戏管理器引用
    AllyInstance &self = static_cast<AllyInstance &>(baseSelf);
    auto &enemies = gameManager.getGameEntities().enemies;
    Renderer &renderer = gameManager.getRenderer();
    auto rng = QRandomGenerator::global();

    // ---------- 1. 受击检测与MP增加 ----------
    if (self.getCurrentHp() < m_lastRecordedHp)
    {
        // 只有受击冷却结束才能加MP
        if (m_lastNotHurtTimer <= 0.0)
        {
            self.currentMp += 20;     // 每次被攻击恢复20MP
            m_lastNotHurtTimer = 0.5; // 重置受击冷却0.5秒
        }
    }
    m_lastRecordedHp = self.getCurrentHp(); // 更新血量记录

    // 减少受击冷却
    if (m_lastNotHurtTimer > 0.0)
        m_lastNotHurtTimer -= dt;

    // 减少行动冷却
    if (m_cooldown > 0.0)
        m_cooldown -= dt;

    // ---------- 2. MP满进入O状态 ----------
    if (m_state.state == HachimiState::DoWhat && self.currentMp >= self.maxMp)
    {
        self.currentMp -= self.maxMp; // 消耗全部MP
        m_state.state = HachimiState::O;
        m_state.attackedRounds = 0; // 重置攻击轮数
        m_cooldown = 0.0;           // 立即行动
        gameManager.splashText("哈！", self.transform.x, self.transform.y, "#FF8C32");
        // O状态刚进入时，强制下一次tick随机选敌
        m_target = nullptr; // 标记需要重新选敌
    }

    // ---------- 3. Ha状态计时与循环控制 ----------
    if (m_state.state == HachimiState::Ha)
    {
        m_state.m_haStateTimer += dt;
        if (m_state.m_haStateTimer >= 0.5)
        {
            m_state.attackedRounds++; // 完成一轮攻击
            if (m_state.attackedRounds >= 5)
            {
                // 满5轮，回到DoWhat
                m_state.reset();
                m_target = nullptr;
                m_cooldown = 1.0; // 可以给一个回转冷却
            }
            else
            {
                // 未满5轮，返回O状态继续循环
                m_state.state = HachimiState::O;
                m_state.m_haStateTimer = 0.0;
                m_cooldown = 0.0;
                m_target = nullptr; // 下次移动重新随机选敌
            }
        }
        // Ha状态下不进行索敌和移动（在原地释放）
        return;
    }

    // ---------- 4. 索敌 ----------
    if (m_cooldown > 0.0)
        return; // 行动冷却中，不移动不攻击

    if (m_state.state == HachimiState::O)
    {
        // O状态：目标无效时随机选择一个敌人
        if (!m_target || !m_target->isAlive)
            findRandomTarget(enemies, self, rng);
    }
    else if (m_state.state == HachimiState::DoWhat)
    {
        findNearestTarget(enemies, self);
    }

    // ---------- 5. 移动与状态转换 ----------
    updatePos(dt, self, enemies, gameManager);
}

//% 索敌
void HachimiAlly::findNearestTarget(const std::vector<EnemyInstance *> &enemies,
                                    const AllyInstance &self)
{

    // 找到最近敌人
    double minDist = std::numeric_limits<double>::max();
    for (auto &e : enemies)
    {
        if (!e->isAlive)
            continue;
        double dx = e->transform.x - self.transform.x;
        double dy = e->transform.y - self.transform.y;
        double d = std::abs(dx) + std::abs(dy); // 曼哈顿距离
        if (d < minDist)
        {
            minDist = d;
            m_target = e; // 记录最近敌人到成员变量
        }
    }
    return;
}

void HachimiAlly::findRandomTarget(const std::vector<EnemyInstance *> &enemies,
                                   const AllyInstance &self, QRandomGenerator *rng)
{
    // 收集所有存活的敌人
    std::vector<EnemyInstance *> aliveEnemies;
    for (auto *e : enemies)
    {
        if (e->isAlive)
            aliveEnemies.push_back(e);
    }

    if (aliveEnemies.empty())
    {
        m_target = nullptr;
        return;
    }

    // 随机挑选幸运观众
    int idx = rng->bounded(static_cast<int>(aliveEnemies.size()));
    m_target = aliveEnemies[idx];
}
// % 攻击
void HachimiAlly::performHaAttack(AllyInstance &self, GameManager &gameManager)
{
    auto &enemies = gameManager.getGameEntities().enemies;
    Renderer &renderer = gameManager.getRenderer();
    QRandomGenerator *rng = QRandomGenerator::global();

    const double range = 100.0;                  // 哈气范围半径
    double haDamage = self.atk.getFinal() * 1.5; // 哈气伤害倍率

    renderer.queueSplash("哈！", self.transform.x, self.transform.y, "#FF8C32");
    for (auto *enemy : enemies)
    {
        if (!enemy->isAlive)
            continue;
        double dx = enemy->transform.x - self.transform.x;
        double dy = enemy->transform.y - self.transform.y;
        if (dx * dx + dy * dy <= range * range)
        {
            enemy->dealDamage(haDamage, self,
                              DamageType{DamageType::Physical, QColor("#FF8C32")});
            if (!enemy->isAlive)
            {
                int gold = (rng->bounded(10) == 0) ? enemy->baseGoldReward : 0;
                gameManager.addPendingRewards(gold, enemy->baseExpReward);
            }
        }
    }
}

// % 亡语
void HachimiAlly::onDeath(AllyInstance *self)
{
    print("哈基米坠机，触发亡语");
    // 处理哈基米死亡时的逻辑
    for (auto *ally : m_gameManager->getGameEntities().allies)
    {
        if (ally && ally->isAlive)
        {
            // 给其他角色加buff
            ally->baseAttackSpeed *= 2; // 加攻速
            QTimer::singleShot(3000, [ally]()
                               {
                                   if (ally && ally->isAlive)
                                       ally->baseAttackSpeed /= 2; // 30秒后恢复
                               });
        }
    }
    print(QString("allies 数量: %1").arg(m_gameManager->getGameEntities().allies.size()));
}

// % 走位
void HachimiAlly::updatePos(double dt, AllyInstance &self, const std::vector<EnemyInstance *> &enemies, GameManager &gameManager)
{
    if (!m_target || !m_target->isAlive)
        return;

    // 稍微计算一下位置
    double dx = m_target->transform.x - self.transform.x;
    double dy = m_target->transform.y - self.transform.y;
    double dist = std::sqrt(dx * dx + dy * dy);
    if (dist < 0.001)
        return;

    double dirX = dx / dist;
    double dirY = dy / dist;
    double moveSpeed = 150.0;

    switch (m_state.state)
    {
    case HachimiState::DoWhat:
        if (dist > 200.0) // 靠近到200px范围
        {
            self.transform.x += dirX * moveSpeed * dt;
            self.transform.y += dirY * moveSpeed * dt;
        }
        else if (dist < 75.0)
        {
            // 远离到100px范围
            self.transform.x -= dirX * moveSpeed * dt;
            self.transform.y -= dirY * moveSpeed * dt;
        }
        else
        {
            // 绕圈
            self.transform.x += dirY * moveSpeed * dt;
            self.transform.y -= dirX * moveSpeed * dt;
        }
        break;

    case HachimiState::O:
        if (dist > 200.0)
        {
            // 瞬移到目标100px处
            double overshoot = dist - 100.0;
            self.transform.x += dirX * overshoot;
            self.transform.y += dirY * overshoot;
        }
        else if (dist > 100.0)
        {
            // 快速靠近
            self.transform.x += dirX * 2.0 * moveSpeed * dt;
            self.transform.y += dirY * 2.0 * moveSpeed * dt;
        }
        else
        {
            // 进入Ha状态，立即造成范围伤害
            m_state.state = HachimiState::Ha;
            m_state.m_haStateTimer = 0.0;
            performHaAttack(self, gameManager);
        }
        break;

    case HachimiState::Ha:
        // 不移动
        break;
    }
}
// % 重写多状态材质
QString HachimiAlly::getTexturePath(const AllyInstance &self) const
{
    switch (m_state.state)
    {
    case HachimiState::DoWhat:
        return ":/texture/entity/hachimi/hachimi_dowhat.png";
    case HachimiState::Ha:
        return ":/texture/entity/hachimi/hachimi_ha.png";
    case HachimiState::O:
        return ":/texture/entity/hachimi/hachimi_o.png";
    default:
        return ":/texture/entity/hachimi/hachimi_dowhat.png";
    }
}
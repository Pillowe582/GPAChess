#include "game_manager.h"
#include "database_manager.h"
#include "print.h"
#include "entity/ally_behavior.h"
#include "entity/enemy_behavior.h"

#include <algorithm>
#include <QRandomGenerator>

GameManager::GameManager(QObject *parent)
    : QObject(parent), m_tickTimer(new QTimer(this)), m_tickIntervalMs(GAME_TICK_INTERVAL_MS),
      m_roundNumber(1), m_phase(RoundPhase::Prepare), m_towerHp(BASE_TOWER_HP * m_towerHpMultiplier),
      m_timeAccumulator(0.0), m_gameSeed(12345), m_rng(12345)
{
    m_tickTimer->setInterval(m_tickIntervalMs);
    connect(m_tickTimer, &QTimer::timeout, this, &GameManager::onTick);
}

/// @brief 整局游戏初始化
void GameManager::initialize()
{
    // 生成本局随机种子
    m_gameSeed = QRandomGenerator::global()->generate();
    m_rng.seed(m_gameSeed);
    print(QString("Game seed: %1").arg(m_gameSeed));

    // 重置玩家资源
    m_player.gold = 10;
    m_player.exp = 0;
    m_player.ownedChesses.clear();

    // 开局免费给n个随机角色放在备战席
    if (m_database)
    {
        const auto &pool = m_database->allChessConfigs();
        const int starterCount = 2;
        for (int i = 0; i < starterCount && !pool.empty(); ++i)
        {
            int idx = m_rng.bounded(static_cast<int>(pool.size()));
            ChessInstance inst(pool[idx]);
            inst.deployed = false;
            inst.benchSlot = i;
            inst.behavior.reset(createAllyBehavior(inst.behaviorId));
            m_player.ownedChesses.push_back(std::move(inst));
        }
    }

    m_enemies.clear();
    m_enemyConfigs.clear();
    m_roundNumber = 1;
    m_maxTowerHp = BASE_TOWER_HP * m_towerHpMultiplier;
    m_towerHp = m_maxTowerHp;
    m_roundStartGold = 0;
    m_roundStartExp = 0;
    m_weightedGpaSum = 0.0;
    m_totalCredits = 0;
    m_phase = RoundPhase::Prepare;
    emit phaseChanged(m_phase);
}

/// @brief 轮次开始
/// @param roundNumber
void GameManager::startRound(int roundNumber)
{
    m_roundNumber = roundNumber;
    // 快照回合开始时的金币/经验
    m_roundStartGold = m_player.gold;
    m_roundStartExp = m_player.exp;
    // 每回合塔满血
    m_towerHp = BASE_TOWER_HP * m_towerHpMultiplier;
    m_towerAttackCooldown = 0.0;
    transitionPhase(RoundPhase::Combat);
    spawnEnemies(m_roundNumber);
    m_tickTimer->start();
}

/// @brief 轮次结束
void GameManager::stopRound()
{
    if (m_tickTimer->isActive())
        m_tickTimer->stop();
}

/// @brief 处理从结算界面进入下一轮准备阶段的过渡
void GameManager::nextRound()
{
    if (m_phase != RoundPhase::ShowResult)
        return;

    transitionPhase(RoundPhase::Finish);
    // 结算：应用积累收益 + 底薪 + 本回合学分绩
    m_player.gold += m_pendingGold + m_guaranteedGold;
    m_player.exp += m_pendingExp;
    double roundGpa = getRoundGpa();
    int credit = getRoundCredit(m_roundNumber);
    m_weightedGpaSum += roundGpa * credit;
    m_totalCredits += credit;
    m_pendingGold = 0;
    m_pendingExp = 0;
    resetUnitsForNextRound();
    ++m_roundNumber;

    if (m_roundNumber > m_maxRounds)
    {
        double finalGpa = m_totalCredits > 0 ? m_weightedGpaSum / m_totalCredits : 0.0;
        emit gameOver(finalGpa, m_player.gold, m_player.exp);
        print(QString("Game over! Final GPA: %1").arg(finalGpa, 0, 'f', 2));
        return;
    }
    transitionPhase(RoundPhase::Prepare);
}

/// @brief 重置当前轮次所有单位
void GameManager::resetUnitsForNextRound()
{
    for (auto &unit : m_player.ownedChesses)
        unit.resetStatus();
    m_enemies.clear();
    m_towerAttackCooldown = 0.0;
    m_timeAccumulator = 0.0;
    print("All units reset for next round, formation preserved");
}

/// @brief 过渡到新的回合阶段，顺便发出信号
/// @param newPhase
void GameManager::transitionPhase(RoundPhase newPhase)
{
    if (m_phase == newPhase)
        return;
    m_phase = newPhase;
    emit phaseChanged(m_phase);
}

void GameManager::spawnEnemies(int roundNumber)
{
    m_enemies.clear();

    int count = 3 + (roundNumber > 5 ? 1 : 0);
    auto picked = pickRandomEnemies(count, roundNumber);

    int uidBase = 1000 + roundNumber * 100;
    for (size_t i = 0; i < picked.size(); ++i)
    {
        m_enemies.emplace_back(uidBase + static_cast<int>(i), picked[i], false, roundNumber);
        EnemyInstance &e = m_enemies.back();
        e.behavior.reset(createEnemyBehavior(picked[i].behaviorId));
        e.posX = 1180.0 + (static_cast<double>(i % 3) * 140.0);
        e.posY = 160.0 + (static_cast<double>(i / 3) * 180.0);
    }
}

std::vector<EnemyConfig> GameManager::pickRandomEnemies(int count, int roundNumber)
{
    std::vector<EnemyConfig> result;
    if (!m_database)
        return result;

    const auto &pool = m_database->allEnemyConfigs();
    if (pool.empty())
        return result;

    result.reserve(count);
    for (int i = 0; i < count; ++i)
    {
        int idx = m_rng.bounded(static_cast<int>(pool.size()));
        EnemyConfig cfg = pool[idx];
        cfg.baseHp += roundNumber * 10;
        cfg.baseAtk += roundNumber * 3;
        result.push_back(cfg);
    }
    return result;
}

void GameManager::onTick()
{
    // delta in seconds
    double delta = m_tickIntervalMs / 1000.0;
    m_timeAccumulator += delta;
    executeAttackCycle(delta);
    emit tick();

    bool victory = false;
    if (checkCombatEndConditions(victory))
    {
        stopRound();
        transitionPhase(RoundPhase::ShowResult);
        emit roundEnded(victory);
    }
}

void GameManager::executeAttackCycle(double deltaSeconds)
{
    if (m_towerAttackCooldown > 0.0)
        m_towerAttackCooldown = std::max(0.0, m_towerAttackCooldown - deltaSeconds);

    auto rng = QRandomGenerator::global();
    auto jitter = [rng]() -> double
    { return (rng->generateDouble() - 0.5) * 30.0; };

    // ====== 我方单位攻击（behavior 驱动） ======
    for (auto &ally : m_player.ownedChesses)
    {
        if (!ally.isAlive || !ally.deployed || !ally.behavior)
            continue;
        ally.skillCooldownRemaining = std::max(0.0, ally.skillCooldownRemaining - deltaSeconds);
        if (ally.skillCooldownRemaining > 0.0)
            continue;

        EnemyInstance *target = ally.behavior->findTarget(m_enemies, ally);
        if (!target)
            continue;

        int dmg = ally.behavior->getDamage(ally);
        target->takeDamage(dmg);
        emit floatingText(QString("-%1").arg(dmg),
                          target->posX + jitter(), target->posY - 20.0 + jitter(),
                          ally.behavior->damageColor());
        print(QString("Ally %1 hits Enemy %2 for %3 dmg").arg(ally.uuid).arg(target->uuid).arg(dmg));

        if (!target->isAlive)
        {
            if (rng->bounded(10) == 0)
                m_pendingGold += target->baseGoldReward;
            m_pendingExp += target->baseExpReward;
            print(QString("Enemy %1 died").arg(target->uuid));
        }

        ally.skillCooldownRemaining = ally.behavior->getInterval(ally);
    }

    // ====== 敌方单位攻击（behavior 驱动） ======
    for (auto &enemy : m_enemies)
    {
        if (!enemy.isAlive || !enemy.behavior)
            continue;
        enemy.skillCooldownRemaining = std::max(0.0, enemy.skillCooldownRemaining - deltaSeconds);
        if (enemy.skillCooldownRemaining > 0.0)
            continue;

        ChessInstance *target = enemy.behavior->findTarget(m_player.ownedChesses, enemy);
        if (target)
        {
            int dmg = enemy.behavior->getDamage(enemy);
            target->takeDamage(dmg);
            emit floatingText(QString("-%1").arg(dmg),
                              target->posX + jitter(), target->posY - 20.0 + jitter(),
                              enemy.behavior->damageColor());
            print(QString("Enemy %1 hits Ally %2 for %3 dmg").arg(enemy.uuid).arg(target->uuid).arg(dmg));
            if (!target->isAlive)
                print(QString("Ally %1 died").arg(target->uuid));
            enemy.skillCooldownRemaining = enemy.behavior->getInterval(enemy);
        }
        else
        {
            int dmg = enemy.behavior->getDamage(enemy);
            m_towerHp -= dmg;
            emit floatingText(QString("-%1").arg(dmg),
                              80.0 + jitter(), 400.0 + jitter(), "#FFFFFF");
            print(QString("Enemy %1 hits tower for %2 dmg, towerHP=%3").arg(enemy.uuid).arg(dmg).arg(m_towerHp));
            enemy.skillCooldownRemaining = enemy.behavior->getInterval(enemy);
        }
    }

    // ====== 塔攻击 ======
    if (m_towerAttackCooldown <= 0.0 && m_towerHp > 0)
    {
        auto it = std::find_if(m_enemies.begin(), m_enemies.end(),
                               [](const EnemyInstance &e)
                               { return e.isAlive; });
        if (it != m_enemies.end())
        {
            double hpRatio = static_cast<double>(m_towerHp) / static_cast<double>(m_maxTowerHp);
            double multiplier = 1.0 + (1.0 - hpRatio) * 3.0;
            int baseDmg = 30;
            int dmg = static_cast<int>(baseDmg * multiplier);
            it->takeDamage(dmg);
            emit floatingText(QString("-%1").arg(dmg),
                              it->posX + jitter(), it->posY - 20.0 + jitter(), "#4696FF");
            print(QString("Tower hits Enemy %1 for %2 dmg (x%3)").arg(it->uuid).arg(dmg).arg(multiplier, 0, 'f', 2));

            if (!it->isAlive)
            {
                if (rng->bounded(10) == 0)
                    m_pendingGold += it->baseGoldReward;
                m_pendingExp += it->baseExpReward;
                print(QString("Tower killed Enemy %1").arg(it->uuid));
            }
            m_towerAttackCooldown = 1.5;
        }
    }
}

bool GameManager::checkCombatEndConditions(bool &outVictory)
{
    bool anyEnemyAlive = std::any_of(m_enemies.begin(), m_enemies.end(), [](const EnemyInstance &e)
                                     { return e.isAlive; });

    // 敌方全灭 → 胜利（无论我方是否存活，塔可以收尾）
    if (!anyEnemyAlive)
    {
        outVictory = true;
        return true;
    }

    // 塔被摧毁 → 失败
    if (m_towerHp <= 0)
    {
        outVictory = false;
        return true;
    }

    outVictory = false;
    return false;
}

void GameManager::checkAndMergeStars()
{
    auto &units = m_player.ownedChesses;
    bool merged = true;
    while (merged)
    {
        merged = false;
        for (size_t i = 0; i < units.size(); ++i)
        {
            if (!units[i].isAlive)
                continue;
            int cfgId = units[i].configId;
            int star = units[i].starLevel;

            std::vector<size_t> same;
            for (size_t j = 0; j < units.size(); ++j)
            {
                if (i == j)
                    continue;
                if (!units[j].isAlive)
                    continue;
                if (units[j].configId == cfgId && units[j].starLevel == star)
                    same.push_back(j);
                if (same.size() >= 2)
                    break;
            }

            if (same.size() >= 2)
            {
                size_t a = i, b = same[0], c = same[1];
                // 合并目标优先：战场上的 > 备战席靠左的
                size_t target = a;
                auto rank = [&](size_t idx) -> int
                {
                    if (units[idx].deployed)
                        return 0;
                    return units[idx].benchSlot + 1;
                };
                if (rank(b) < rank(target))
                    target = b;
                if (rank(c) < rank(target))
                    target = c;

                units[target].starLevel++;
                units[target].calculateBaseStatsByStar();
                units[target].resetStatus();

                size_t d1 = (target == a) ? b : a;
                size_t d2 = (target == c) ? c : ((target == b) ? c : b);
                if (d1 == d2)
                {
                    // 如果d1和d2指向同一元素，调整
                    for (size_t s : same)
                        if (s != d1)
                        {
                            d2 = s;
                            break;
                        }
                }
                if (d1 > d2)
                    std::swap(d1, d2);
                units.erase(units.begin() + static_cast<long long>(d2));
                units.erase(units.begin() + static_cast<long long>(d1));

                print(QString("Merged 3x %1★%2 → %3★%4 (uuid=%5)")
                          .arg(star)
                          .arg(cfgId)
                          .arg(star + 1)
                          .arg(cfgId)
                          .arg(units[target].uuid));
                merged = true;
                break;
            }
        }
    }
}

/// @brief 卖出单位
/// @param uuid
/// @return 卖出获得的金币数量
int GameManager::sellUnit(int uuid)
{
    auto &units = m_player.ownedChesses;
    for (size_t i = 0; i < units.size(); ++i)
    {
        if (units[i].uuid == uuid)
        {
            int star = units[i].starLevel;
            int cost = units[i].cost;
            int refund = cost;

            // 卖出价格 = 基础费用 × (2^星级 - 1)
            for (int s = 1; s < star; ++s)
                refund *= 3;

            m_player.gold += refund;
            print(QString("Sold %1★%2for %4 gold")
                      .arg(star)
                      .arg(uuid)
                      .arg(refund));

            units.erase(units.begin() + static_cast<long long>(i));
            return refund;
        }
    }
    return 0;
}
GameManager::~GameManager()
{
    if (m_tickTimer->isActive())
        m_tickTimer->stop();
}
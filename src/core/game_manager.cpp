#include "game_manager.h"
#include "print.h"

#include <algorithm>
#include <QRandomGenerator>

GameManager::GameManager(QObject *parent)
    : QObject(parent), m_tickTimer(new QTimer(this)), m_tickIntervalMs(50), m_roundNumber(1), m_phase(RoundPhase::Prepare), m_towerHp(100), m_timeAccumulator(0.0)
{
    m_tickTimer->setInterval(m_tickIntervalMs);
    connect(m_tickTimer, &QTimer::timeout, this, &GameManager::onTick);
}

GameManager::~GameManager()
{
    if (m_tickTimer->isActive())
        m_tickTimer->stop();
}

void GameManager::initialize()
{
    // 重置玩家资源
    m_player.gold = 10;
    m_player.exp = 0;
    m_player.ownedChesses.clear();

    // 放入 5 个默认占位角色，全部部署到战场（后续商店上线后可改为从备战席部署）
    m_player.ownedChesses.reserve(5);
    for (int i = 0; i < 5; ++i)
    {
        ChessConfig cfg;
        cfg.configId = i + 1;
        cfg.name = QString("Role%1").arg(i + 1);
        cfg.cost = 1;
        cfg.baseHp = 30 + i * 5;
        cfg.baseAtk = 8 + i * 2;
        cfg.baseDef = 1 + (i % 2);
        cfg.hpGrowthMultiplier = 1.10f;
        cfg.atkGrowthMultiplier = 1.08f;
        cfg.attackRange = 1;
        cfg.baseAttackSpeed = 1.0f + 0.1f * i;
        cfg.maxMp = 100;
        cfg.speed = 1.0;
        cfg.description = QString("Default placeholder hero %1").arg(i + 1);
        cfg.bonds = {};
        m_player.ownedChesses.emplace_back(cfg);
        // 默认全部部署到战场
        ChessInstance &inst = m_player.ownedChesses.back();
        inst.deployed = true;
        inst.benchSlot = -1;
        inst.posX = 260.0 + (static_cast<double>(i % 3) * 140.0);
        inst.posY = 160.0 + (static_cast<double>(i / 3) * 180.0);
    }

    m_enemies.clear();
    m_enemyConfigs.clear();
    m_attackCooldownRemaining.clear();
    m_roundNumber = 1;
    m_maxTowerHp = 100;
    m_towerHp = m_maxTowerHp;
    m_roundStartGold = 0;
    m_roundStartExp = 0;
    m_phase = RoundPhase::Prepare;
    emit phaseChanged(m_phase);
}

void GameManager::startRound(int roundNumber)
{
    m_roundNumber = roundNumber;
    // 快照回合开始时的金币/经验
    m_roundStartGold = m_player.gold;
    m_roundStartExp = m_player.exp;
    // 每回合塔满血
    m_maxTowerHp = 100;
    m_towerHp = m_maxTowerHp;
    m_towerAttackCooldown = 0.0;
    transitionPhase(RoundPhase::Combat);
    spawnEnemies(m_roundNumber);
    m_tickTimer->start();
}

void GameManager::stopRound()
{
    if (m_tickTimer->isActive())
        m_tickTimer->stop();
}

void GameManager::nextRound()
{
    // ShowResult → Finish → Prepare
    if (m_phase != RoundPhase::ShowResult)
        return;

    transitionPhase(RoundPhase::Finish);
    resetUnitsForNextRound();
    ++m_roundNumber;
    transitionPhase(RoundPhase::Prepare);
}

void GameManager::resetUnitsForNextRound()
{
    // 复活所有单位、重置HP/MP/冷却，但保持 deployed/pos/benchSlot
    for (auto &unit : m_player.ownedChesses)
    {
        unit.resetStatus(); // HP/MP/isAlive 恢复
    }
    m_attackCooldownRemaining.clear();
    m_towerAttackCooldown = 0.0;
    m_timeAccumulator = 0.0;
    print("All units reset for next round, formation preserved");
}

void GameManager::transitionPhase(RoundPhase newPhase)
{
    if (m_phase == newPhase)
        return;
    m_phase = newPhase;
    emit phaseChanged(m_phase);
}

// Create a few simple enemies for the round
void GameManager::spawnEnemies(int roundNumber)
{
    m_enemies.clear();
    m_enemyConfigs.clear();

    // For a minimal workable demo we create 3 enemies with increasing stats
    for (int i = 0; i < 3; ++i)
    {
        EnemyConfig cfg;
        cfg.configId = i + 1;
        cfg.name = QString("Enemy%1").arg(i + 1);
        cfg.baseHp = 20 + roundNumber * 5 + i * 5;
        cfg.baseAtk = 5 + roundNumber * 2 + i;
        cfg.baseDef = 0;
        cfg.hpGrowthMultiplier = 1.0f;
        cfg.atkGrowthMultiplier = 1.0f;
        cfg.baseAttackSpeed = 1.0f;
        cfg.speed = 1.0;
        cfg.maxMp = 0;
        cfg.baseGoldReward = 1;
        cfg.baseExpReward = 1;
        cfg.description = "";
        m_enemyConfigs.push_back(cfg);
    }

    int uidBase = 1000;
    for (size_t i = 0; i < m_enemyConfigs.size(); ++i)
    {
        m_enemies.emplace_back(uidBase + static_cast<int>(i), m_enemyConfigs[i], false, roundNumber);
        // 设置敌人场景坐标（与 refreshBattleGround 中的渲染位置一致）
        EnemyInstance &e = m_enemies.back();
        e.posX = 1180.0 + (static_cast<double>(i % 3) * 140.0);
        e.posY = 160.0 + (static_cast<double>(i / 3) * 180.0);
    }
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
    // 更新所有冷却
    for (auto &kv : m_attackCooldownRemaining)
    {
        if (kv.second > 0.0)
            kv.second = std::max(0.0, kv.second - deltaSeconds);
    }
    if (m_towerAttackCooldown > 0.0)
        m_towerAttackCooldown = std::max(0.0, m_towerAttackCooldown - deltaSeconds);

    auto rng = QRandomGenerator::global();
    // 随机偏移辅助
    auto jitter = [rng]() -> double
    { return (rng->generateDouble() - 0.5) * 30.0; };

    // ====== 我方单位攻击 ======
    for (auto &ally : m_player.ownedChesses)
    {
        if (!ally.isAlive || !ally.deployed)
            continue;

        int uid = ally.uuid;
        double &cd = m_attackCooldownRemaining[uid];
        if (cd > 0.0)
            continue;

        auto it = std::find_if(m_enemies.begin(), m_enemies.end(), [](const EnemyInstance &e)
                               { return e.isAlive; });
        if (it != m_enemies.end())
        {
            int dmg = ally.atk.getFinal();
            it->takeDamage(dmg);
            emit floatingText(QString("-%1").arg(dmg),
                              it->posX + jitter(), it->posY - 20.0 + jitter(),
                              255, 80, 80);
            print(QString("Ally %1 hits Enemy %2 for %3 dmg").arg(ally.uuid).arg(it->uuid).arg(dmg));

            if (!it->isAlive)
            {
                // 10% 概率掉落金币
                if (rng->bounded(10) == 0)
                    m_player.gold += it->baseGoldReward;
                m_player.exp += it->baseExpReward;
                print(QString("Enemy %1 died").arg(it->uuid));
            }

            int atkSp = ally.attackSpeed.getFinal();
            double interval = atkSp > 0 ? (1.0 / atkSp) : 1.0;
            cd = interval;
        }
    }

    // ====== 敌方单位攻击 ======
    for (auto &enemy : m_enemies)
    {
        if (!enemy.isAlive)
            continue;

        int uid = enemy.uuid;
        double &cd = m_attackCooldownRemaining[uid];
        if (cd > 0.0)
            continue;

        auto it = std::find_if(m_player.ownedChesses.begin(), m_player.ownedChesses.end(),
                               [](const ChessInstance &c)
                               { return c.isAlive && c.deployed; });
        if (it != m_player.ownedChesses.end())
        {
            int dmg = enemy.atk.getFinal();
            it->takeDamage(dmg);
            emit floatingText(QString("-%1").arg(dmg),
                              it->posX + jitter(), it->posY - 20.0 + jitter(),
                              255, 180, 50);
            print(QString("Enemy %1 hits Ally %2 for %3 dmg").arg(enemy.uuid).arg(it->uuid).arg(dmg));
            if (!it->isAlive)
                print(QString("Ally %1 died").arg(it->uuid));
        }
        else
        {
            int dmg = enemy.atk.getFinal();
            m_towerHp -= dmg;
            emit floatingText(QString("-%1").arg(dmg),
                              80.0 + jitter(), 400.0 + jitter(),
                              255, 220, 60);
            print(QString("Enemy %1 hits tower for %2 dmg, towerHP=%3").arg(enemy.uuid).arg(dmg).arg(m_towerHp));
        }

        int atkSp = enemy.attackSpeed.getFinal();
        double interval = atkSp > 0 ? (1.0 / atkSp) : 1.0;
        cd = interval;
    }

    // ====== 塔攻击 ======
    if (m_towerAttackCooldown <= 0.0 && m_towerHp > 0)
    {
        auto it = std::find_if(m_enemies.begin(), m_enemies.end(), [](const EnemyInstance &e)
                               { return e.isAlive; });
        if (it != m_enemies.end())
        {
            double hpRatio = static_cast<double>(m_towerHp) / static_cast<double>(m_maxTowerHp);
            double multiplier = 1.0 + (1.0 - hpRatio) * 3.0;
            int baseDmg = 15;
            int dmg = static_cast<int>(baseDmg * multiplier);
            it->takeDamage(dmg);
            // 塔伤害跳字：纯蓝色，无"塔"前缀
            emit floatingText(QString("-%1").arg(dmg),
                              it->posX + jitter(), it->posY - 20.0 + jitter(),
                              70, 150, 255);
            print(QString("Tower hits Enemy %1 for %2 dmg (x%3)").arg(it->uuid).arg(dmg).arg(multiplier, 0, 'f', 2));

            if (!it->isAlive)
            {
                if (rng->bounded(10) == 0)
                    m_player.gold += it->baseGoldReward;
                m_player.exp += it->baseExpReward;
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

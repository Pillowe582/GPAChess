#include "game_manager.h"
#include "database_manager.h"
#include "print.h"
#include "ally_behavior.h"
#include "enemy_behavior.h"
#include "allies/tower/tower_behavior.h"
#include "renderer.h"

#include <algorithm>
#include <cmath>
#include <QRandomGenerator>
#include <QCoreApplication>

GameManager::GameManager(QObject *parent)
    : QObject(parent), m_tickTimer(new QTimer(this)),
      m_roundNumber(1), m_phase(RoundPhase::Prepare),
      m_timeAccumulator(0.0), m_gameSeed(12345), m_rng(12345)
{
    m_tickTimer->setInterval(GAME_TICK_INTERVAL_MS);
    connect(m_tickTimer, &QTimer::timeout, this, &GameManager::onTick);
}

// % 全局初始化
/// @brief 整局游戏初始化
void GameManager::initialize()
{
    print("开始初始化游戏管理器");
    /// 设置图鉴数据库引用
    m_database = new DatabaseManager(QCoreApplication::applicationDirPath(), this);
    if (!m_database)
    {
        print("数据库管理器初始化失败！");
        return;
    }
    print("数据库管理器初始化成功");
    // 生成本局随机种子
    m_gameSeed = QRandomGenerator::global()->generate();
    m_rng.seed(m_gameSeed);
    print(QString("Seed: %1").arg(m_gameSeed));

    // 设置本局初始值
    m_roundNumber = 1;
    m_roundStartGold = 0;
    m_roundStartExp = 0;
    m_weightedGpaSum = 0.0;
    m_totalCredits = 0;
    m_phase = RoundPhase::Prepare;
    m_enemies.clear();
    m_enemyConfigs.clear();

    // 生成本局所有回合信息
    generateRoundInfos(m_roundInfos, m_maxRounds);

    // 重置玩家资源
    m_player.gold = 100;
    m_player.exp = 0;
    m_player.ownedChesses.clear();

    // 创建塔作为特殊棋子
    {
        AllyConfig towerCfg;
        towerCfg.configId = -1; // 塔的唯一ID，防止被合并
        towerCfg.name = "Tower";
        towerCfg.baseHp = BASE_TOWER_HP * m_towerHpMultiplier;
        towerCfg.baseAtk = 100;
        towerCfg.baseDef = 100;
        towerCfg.speed = 0;
        towerCfg.behaviorId = -1; // 塔专用
        auto towerInst = std::make_unique<ChessInstance>(towerCfg, this);
        towerInst->isTower = true;
        towerInst->deployed = true;
        towerInst->transform.x = 200.0;
        towerInst->transform.y = 400.0;
        towerInst->behavior.reset(new TowerBehavior());
        m_player.ownedChesses.push_back(std::move(towerInst));

        // 遍历已拥有的棋子，找到塔实例并保存指针
        for (auto &chess : m_player.ownedChesses)
            if (chess->isTower)
            {
                m_tower = chess.get();
                break;
            }
    }

    // 开局免费给n个随机角色放在备战席
    const auto &pool = m_database->allAllyConfigs();
    const int starterCount = 2;
    for (int i = 0; i < starterCount && !pool.empty(); ++i)
    {
        int idx = m_rng.bounded(static_cast<int>(pool.size()));
        auto inst = std::make_unique<ChessInstance>(pool[idx], this);

        inst->deployed = false;
        inst->benchSlot = i;
        inst->behavior.reset(createAllyBehavior(inst->behaviorId));
        m_player.ownedChesses.push_back(std::move(inst));
    }
    print("游戏管理器初始化完成");
    emit phaseChanged(m_phase);
}

/// @brief 生成本局所有回合信息
/// @param roundInfos 存入回合信息向量
/// @param maxRounds 总回合数
void GameManager::generateRoundInfos(std::vector<RoundInfo> &roundInfos, int maxRounds)
{
    roundInfos.clear();
    for (int i = 1; i <= maxRounds; ++i)
    {
        RoundInfo info;
        info.roundNumber = i;
        info.creditWorth = generateRoundCredit(i);
        info.electiveEnemies = 3 + (i > 5 ? 1 : 0); // 可选敌人数量随回合增加
        roundInfos.push_back(info);
    }
}
/// @brief 随机出一个回合学分值（1~5cr）
/// @param round 回合数
/// @return 学分值
int GameManager::generateRoundCredit(int round)
{
    return QRandomGenerator::global()->bounded(1, 6);
}

// % 回合逻辑

/// @brief 回合开始
/// @param roundNumber 回合数
void GameManager::startRound(int roundNumber)
{
    m_roundNumber = roundNumber;

    m_tower->baseDef = roundNumber * 200; // 塔防御随回合增加
    // 快照回合开始时的金币/经验
    m_roundStartGold = m_player.gold;
    m_roundStartExp = m_player.exp;
    m_timeAccumulator = 0.0;

    // 重置 elective 追踪状态
    m_electiveEnemiesNotSpawned = m_roundInfos[m_roundNumber].electiveEnemies;
    m_mandatoryEnemiesCleared = false;

    // 快照所有已部署单位的战场位置，用于回合结束后复位
    for (auto &u : m_player.ownedChesses)
    {
        if (u->deployed)
        {
            u->savedPosX = u->transform.x;
            u->savedPosY = u->transform.y;
        }
    }

    transitionPhase(RoundPhase::Combat);
    spawnEnemies(m_roundNumber, m_roundInfos[m_roundNumber].creditWorth + 2, true);

    // ====== 通知所有 behavior：回合开始 ======
    for (const auto &u : m_player.ownedChesses)
        if (u && u->behavior)
            u->behavior->onStart();
    for (auto &e : m_enemies)
        if (e.behavior)
            e.behavior->onStart();

    m_tickTimer->start();
}

/// @brief 回合结束
void GameManager::stopRound()
{
    if (m_tickTimer->isActive())
        m_tickTimer->stop();
    if (m_renderer)
        m_renderer->clearAll();
}

/// @brief 处理从结算界面进入下一轮准备阶段的过渡
void GameManager::nextRound()
{
    if (m_phase != RoundPhase::ShowResult)
        return;

    transitionPhase(RoundPhase::Finish);
    // 结算：应用积累收益 + 底薪
    m_player.gold += m_pendingGold + m_guaranteedGold;
    m_player.exp += m_pendingExp;

    // 计算本回合学分绩，并更新加权平均学分绩
    double roundGpa = getRoundGpa();
    int credit = m_roundInfos[m_roundNumber].creditWorth;
    m_weightedGpaSum += roundGpa * credit;
    m_totalCredits += credit;

    // 重置本回合收益
    m_pendingGold = 0;
    m_pendingExp = 0;

    // 重置所有单位状态，保留阵型
    resetUnitsForNextRound();
    ++m_roundNumber;

    // 如果达到最大回合数，则游戏结束
    if (m_roundNumber > m_maxRounds)
    {
        double finalGpa = m_totalCredits > 0 ? m_weightedGpaSum / m_totalCredits : 0.0;
        emit gameOver(finalGpa);
        print(QString("Game over! Final GPA: %1").arg(finalGpa, 0, 'f', 2));
        return;
    }

    // 进入下一轮准备阶段
    transitionPhase(RoundPhase::Prepare);
}

/// @brief 重置当前轮次所有单位
void GameManager::resetUnitsForNextRound()
{
    for (auto &unit : m_player.ownedChesses)
    {
        unit->resetStatus();
        // 回合结束后，已部署单位回到回合开始时的位置
        if (unit->deployed)
        {
            unit->transform.x = unit->savedPosX;
            unit->transform.y = unit->savedPosY;
        }
    }
    m_enemies.clear();
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

// % 敌方生成逻辑
void GameManager::spawnEnemies(int roundNumber, int count, bool mandatory)
{
    m_enemies.clear();

    // 根据回合数决定敌方数量
    auto picked = pickRandomEnemies(roundNumber, count, mandatory);

    for (size_t i = 0; i < picked.size(); ++i)
    {
        m_enemies.emplace_back(picked[i], false, roundNumber, this);
        EnemyInstance &e = m_enemies.back();
        e.isRequired = mandatory;
        e.behavior.reset(createEnemyBehavior(picked[i].behaviorId));
        e.transform.x = 1180.0 + (static_cast<double>(i % 3) * 140.0);
        e.transform.y = 160.0 + (static_cast<double>(i / 3) * 180.0);
    }
}

std::vector<EnemyConfig> GameManager::pickRandomEnemies(int roundNumber, int count, bool mandatory)
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
        cfg.baseHp += roundNumber * cfg.hpGrowthMultiplier;
        cfg.baseAtk += roundNumber * cfg.atkGrowthMultiplier;
        result.push_back(cfg);
    }
    return result;
}

// % 发出游戏刻信号
void GameManager::onTick()
{
    // delta in seconds
    double delta = GAME_TICK_INTERVAL_MS / 1000.0;
    m_timeAccumulator += delta;
    tickBehaviors(delta);
    emit tick();

    bool victory = false;
    if (checkCombatEndConditions(victory))
    {
        stopRound();
        transitionPhase(RoundPhase::ShowResult);
        emit roundEnded(victory);
    }
}

// % 攻击循环

/// @brief tick 时调用，通知所有单位的行为逻辑
/// @param deltaSeconds 逻辑间隔
void GameManager::tickBehaviors(double deltaSeconds)
{
    if (!m_renderer)
        return;

    // ═══════ 逻辑 Phase ═══════
    // Ally (含塔 —— 塔就是 isTower=true 的普通 ally)
    for (auto &ally : m_player.ownedChesses)
        if (ally->isAlive && ally->deployed && ally->behavior)
            ally->behavior->tick(deltaSeconds, *ally, m_enemies, *m_renderer,
                                 m_pendingGold, m_pendingExp);

    // Enemy
    for (auto &enemy : m_enemies)
        if (enemy.isAlive && enemy.behavior)
            enemy.behavior->tick(deltaSeconds, enemy, m_player.ownedChesses,
                                 *m_renderer, m_pendingGold, m_pendingExp);

    // ═══════ 渲染 Phase ═══════
    m_renderer->beginFrame();
    for (auto &enemy : m_enemies)
        if (enemy.isAlive && enemy.behavior)
            enemy.behavior->renderSelf(enemy, *m_renderer, enemy.transform.x, enemy.transform.y);

    // Ally 渲染由 refreshAllUnits 处理
}

// % 检查战斗结束

/// @brief 检查战斗结束条件
/// @param outVictory 我方是否胜利
/// @return 是否结束战斗
bool GameManager::checkCombatEndConditions(bool &outVictory)
{
    // 检查我方是否还有单位存活
    bool anyAllyAlive = std::any_of(m_player.ownedChesses.begin(), m_player.ownedChesses.end(),
                                    [](const std::unique_ptr<ChessInstance> &ally)
                                    { return ally->isAlive; });
    bool anyEnemyAlive = std::any_of(m_enemies.begin(), m_enemies.end(), [](const EnemyInstance &e)
                                     { return e.isAlive; });

    // 检查是否有必修敌人存活
    bool anyMandatoryAlive = std::any_of(m_enemies.begin(), m_enemies.end(), [](const EnemyInstance &e)
                                         { return e.isAlive && e.isRequired; });

    if (!anyAllyAlive && m_mandatoryEnemiesCleared)
    {
        print("所有我方单位已阵亡，但已取得必修学分");
        outVictory = true;
        return true;
    }
    // 如果必修敌人刚被消灭，标记状态
    if (!anyMandatoryAlive && !m_mandatoryEnemiesCleared)
    {
        m_mandatoryEnemiesCleared = true;
        print("必修敌人已被消灭！");
    }

    // 塔被摧毁 → 失败
    if (m_tower && !m_tower->isAlive)
    {
        print("塔已被摧毁！");
        outVictory = false;
        return true;
    }

    // 敌方全灭时的处理
    if (!anyEnemyAlive)
    {
        // 如果必修敌人已清除，但还有 elective 波次未生成
        if (m_mandatoryEnemiesCleared && m_electiveEnemiesNotSpawned > 0)
        {

            if (anyAllyAlive)
            {
                // 生成选修敌人
                --m_electiveEnemiesNotSpawned;
                print(QString("生成一只选修敌人"));
                int electiveCount = 1;
                auto picked = pickRandomEnemies(m_roundNumber, electiveCount, false);

                for (size_t i = 0; i < picked.size(); ++i)
                {
                    m_enemies.emplace_back(picked[i], false, m_roundNumber, this);
                    EnemyInstance &e = m_enemies.back();
                    e.isRequired = false; // 选修
                    e.behavior.reset(createEnemyBehavior(picked[i].behaviorId));
                    e.transform.x = 1180.0 + (static_cast<double>(i % 3) * 140.0);
                    e.transform.y = 160.0 + (static_cast<double>(i / 3) * 180.0);
                }

                // 不结束战斗，继续战斗
                outVictory = false;
                return false;
            }
        }

        // 所有敌人都消灭了，胜利
        print("所有敌人已被消灭！");
        outVictory = true;
        return true;
    }

    outVictory = false;
    return false;
}
// % 商店逻辑

/// @brief 打开商店窗口
/// @param onClose 商店关闭时的回调函数
void GameManager::openShop(MainWindow *mainWindow, void (MainWindow::*onClose)(int))
{
    // 仅在准备阶段允许打开商店
    if (m_phase != RoundPhase::Prepare)
    {
        print("Warning: Shop can only be opened during Prepare phase!");
        return;
    }

    // 暂停定时器，避免在修改 ownedChesses 时发生竞态条件
    bool wasTimerActive = m_tickTimer->isActive();
    if (wasTimerActive)
        m_tickTimer->stop();

    if (!m_database)
    {
        print("Error: 打开商店失败，DatabaseManager未初始化");
        // 如果原本定时器是激活的，恢复它
        if (wasTimerActive)
            m_tickTimer->start();
        return;
    }
    if (m_shopWindow)
    {
        print("商店窗口已存在，直接显示");
        m_shopWindow->show();
        m_shopWindow->raise();
        m_shopWindow->activateWindow();
        // 恢复定时器状态
        if (wasTimerActive)
            m_tickTimer->start();
        return;
    }
    m_shopWindow = new ShopWindow(m_database, this);

    // 回合结束时自动刷新商店
    connect(this, &GameManager::roundEnded, m_shopWindow, [this]()
            {
                if (m_shopWindow)
                {
                    print("回合结束，商店已刷新");
                    m_shopWindow->refreshShop();
                } });

    // 商店关闭回调
    connect(m_shopWindow, &ShopWindow::shopClosed, this, [this, mainWindow, onClose, wasTimerActive]()
            {
                checkAndMergeStars();
                if (mainWindow && onClose)
                {
                    (mainWindow->*onClose)(MainWindow::RefreshAll);
                }
                // 商店关闭后，如果之前定时器是激活的，则重新启动
                if (wasTimerActive)
                    m_tickTimer->start(); });
    m_shopWindow->refreshShop();
    m_shopWindow->show();
}
// % 升星逻辑
void GameManager::checkAndMergeStars()
{
    auto &units = m_player.ownedChesses;
    bool merged = true;
    while (merged)
    {
        merged = false;
        for (size_t i = 0; i < units.size(); ++i)
        {
            // 跳过塔和死亡单位
            if (!units[i] || !units[i]->isAlive || units[i]->isTower)
                continue;
            int cfgId = units[i]->configId;
            int star = units[i]->starLevel;

            std::vector<size_t> same;
            for (size_t j = 0; j < units.size(); ++j)
            {
                if (i == j)
                    continue;
                if (!units[j] || !units[j]->isAlive || units[j]->isTower)
                    continue;
                if (units[j]->configId == cfgId && units[j]->starLevel == star)
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
                    if (!units[idx])
                        return 999;
                    if (units[idx]->deployed)
                        return 0;
                    return units[idx]->benchSlot + 1;
                };
                if (rank(b) < rank(target))
                    target = b;
                if (rank(c) < rank(target))
                    target = c;

                if (!units[target])
                    continue; // 安全检查
                units[target]->starLevel++;
                units[target]->calculateBaseStatsByStar();
                units[target]->resetStatus();

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

                // 先删除较大索引，再删除较小索引，避免索引偏移
                units.erase(units.begin() + static_cast<long long>(d2));
                units.erase(units.begin() + static_cast<long long>(d1));

                if (units[target])
                {
                    print(QString("Merged 3x %1★%2 → %3★%4 (uuid=%5)")
                              .arg(star)
                              .arg(cfgId)
                              .arg(star + 1)
                              .arg(cfgId)
                              .arg(units[target]->getUuid()));
                }
                merged = true;
                break;
            }
        }
    }
}

//% 售卖单位逻辑
/// @brief 卖出单位
/// @param uuid
/// @return 卖出获得的金币数量
int GameManager::sellUnit(int uuid)
{
    auto &units = m_player.ownedChesses;
    for (size_t i = 0; i < units.size(); ++i)
    {
        if (units[i]->getUuid() == uuid)
        {
            // 禁止出售塔
            if (units[i]->isTower)
            {
                print("Error: Cannot sell the tower!");
                return 0;
            }

            int star = units[i]->starLevel;
            int cost = units[i]->cost;
            int refund = getTotalWorth(star, cost);

            m_player.gold += refund;
            print(QString("Sold %1★%2 for %3 gold")
                      .arg(star)
                      .arg(uuid)
                      .arg(refund));

            units.erase(units.begin() + static_cast<long long>(i));
            return refund;
        }
    }
    return 0;
}

int GameManager::getTowerHp() const
{
    if (m_tower)
        return static_cast<int>(m_tower->currentHp);
    else
    {
        print("Error: 未找到塔");
        return 0;
    }
}

int GameManager::getMaxTowerHp() const
{
    return m_tower ? static_cast<int>(m_tower->maxHp.getFinal()) : BASE_TOWER_HP;
}

GameManager::~GameManager()
{
    if (m_tickTimer->isActive())
        m_tickTimer->stop();
}
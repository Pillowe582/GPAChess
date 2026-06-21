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

    // 重置玩家资源
    m_player.gold = 10;
    m_player.exp = 0;
    m_player.ownedChesses.clear();

    // 开局免费给n个随机角色放在备战席

    const auto &pool = m_database->allAllyConfigs();
    const int starterCount = 2;
    for (int i = 0; i < starterCount && !pool.empty(); ++i)
    {
        int idx = m_rng.bounded(static_cast<int>(pool.size()));
        ChessInstance inst(pool[idx], this);

        // 连接盟友受伤信号到 GameManager
        // connect(&inst, &LivingEntity::receivedDamage, this, &GameManager::receivedDamage);

        inst.deployed = false;
        inst.benchSlot = i;
        inst.behavior.reset(createAllyBehavior(inst.behaviorId));
        m_player.ownedChesses.push_back(std::move(inst));
    }

    // 创建塔作为特殊棋子
    {
        AllyConfig towerCfg;
        towerCfg.name = "Tower";
        towerCfg.baseHp = BASE_TOWER_HP * m_towerHpMultiplier;
        towerCfg.baseAtk = 30;
        towerCfg.speed = 0;
        towerCfg.behaviorId = -1; // 塔专用
        ChessInstance towerInst(towerCfg, this);
        towerInst.isTower = true;
        towerInst.deployed = true;
        towerInst.transform.x = 45.0;
        towerInst.transform.y = 400.0;
        towerInst.behavior.reset(new TowerBehavior());
        m_player.ownedChesses.push_back(std::move(towerInst));
        // 缓存塔指针
        for (auto &c : m_player.ownedChesses)
            if (c.isTower)
            {
                m_tower = &c;
                break;
            }
    }

    m_enemies.clear();
    m_enemyConfigs.clear();
    m_roundNumber = 1;
    m_roundStartGold = 0;
    m_roundStartExp = 0;
    m_weightedGpaSum = 0.0;
    m_totalCredits = 0;
    m_phase = RoundPhase::Prepare;
    emit phaseChanged(m_phase);
}

// % 回合逻辑

/// @brief 回合开始
/// @param roundNumber
void GameManager::startRound(int roundNumber)
{
    m_roundNumber = roundNumber;
    // 快照回合开始时的金币/经验
    m_roundStartGold = m_player.gold;
    m_roundStartExp = m_player.exp;
    // 每回合塔满血（和其他 ally 一起在 resetStatus 中处理）
    m_timeAccumulator = 0.0;

    // 快照所有已部署单位的战场位置，用于回合结束后复位
    for (auto &u : m_player.ownedChesses)
    {
        if (u.deployed)
        {
            u.savedPosX = u.transform.x;
            u.savedPosY = u.transform.y;
        }
    }

    // 先生成敌人，再切换阶段，避免 refreshAllUnits 在 enemies 未初始化时被调用
    spawnEnemies(m_roundNumber);

    transitionPhase(RoundPhase::Combat);

    // ====== 通知所有 behavior：回合开始 ======
    for (auto &u : m_player.ownedChesses)
        if (u.behavior)
            u.behavior->onStart();
    for (auto &e : m_enemies)
        if (e.behavior)
            e.behavior->onStart();
    if (m_tower && m_tower->behavior)
        m_tower->behavior->onStart();

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
        emit gameOver(finalGpa);
        print(QString("Game over! Final GPA: %1").arg(finalGpa, 0, 'f', 2));
        return;
    }
    transitionPhase(RoundPhase::Prepare);
}

/// @brief 重置当前轮次所有单位
void GameManager::resetUnitsForNextRound()
{
    for (auto &unit : m_player.ownedChesses)
    {
        unit.resetStatus();
        // 回合结束后，已部署单位回到回合开始时的位置
        if (unit.deployed)
        {
            unit.transform.x = unit.savedPosX;
            unit.transform.y = unit.savedPosY;
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

//%敌方生成逻辑
void GameManager::spawnEnemies(int roundNumber)
{
    m_enemies.clear();

    int count = 3 + (roundNumber > 5 ? 1 : 0);
    auto picked = pickRandomEnemies(count, roundNumber);

    int uidBase = 1000 + roundNumber * 100;
    for (size_t i = 0; i < picked.size(); ++i)
    {
        m_enemies.emplace_back(picked[i], false, roundNumber, this);
        EnemyInstance &e = m_enemies.back();

        e.behavior.reset(createEnemyBehavior(picked[i].behaviorId));
        e.transform.x = 1180.0 + (static_cast<double>(i % 3) * 140.0);
        e.transform.y = 160.0 + (static_cast<double>(i / 3) * 180.0);
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

// % 发出游戏刻信号
void GameManager::onTick()
{
    // delta in seconds
    double delta = GAME_TICK_INTERVAL_MS / 1000.0;
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

// % 攻击循环
void GameManager::executeAttackCycle(double deltaSeconds)
{
    if (!m_renderer)
        return;

    int towerHpPlaceholder = 0;

    // ═══════ 逻辑 Phase ═══════
    // Ally (含塔 —— 塔就是 isTower=true 的普通 ally)
    for (auto &ally : m_player.ownedChesses)
        if (ally.isAlive && ally.deployed && ally.behavior)
            ally.behavior->tick(deltaSeconds, ally, m_enemies, *m_renderer,
                                m_pendingGold, m_pendingExp);

    // Enemy
    for (auto &enemy : m_enemies)
        if (enemy.isAlive && enemy.behavior)
            enemy.behavior->tick(deltaSeconds, enemy, m_player.ownedChesses,
                                 *m_renderer, towerHpPlaceholder, m_pendingGold, m_pendingExp);

    // ═══════ 渲染 Phase ═══════
    m_renderer->beginFrame();
    for (auto &enemy : m_enemies)
        if (enemy.isAlive && enemy.behavior)
            enemy.behavior->renderSelf(enemy, *m_renderer, enemy.transform.x, enemy.transform.y);

    // Ally 渲染由 refreshAllUnits 处理
}

// % 检查战斗结束
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
    if (m_tower && !m_tower->isAlive)
    {
        outVictory = false;
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

    if (!m_database)
    {
        print("Error: 打开商店失败，DatabaseManager未初始化");
        return;
    }
    if (getCurrentPhase() != RoundPhase::Prepare)
    {
        print("Error: 商店仅能在准备阶段打开");
        return;
    }
    if (m_shopWindow)
    {
        print("商店窗口已存在（这对吗）");
        m_shopWindow->show();
        m_shopWindow->raise();
        m_shopWindow->activateWindow();
        return;
    }
    m_shopWindow = new ShopWindow(m_database, this);

    // 回合结束时自动刷新商店
    connect(this, &GameManager::roundEnded, m_shopWindow, [this]()
            {
                if (m_shopWindow)
                {
                    m_shopWindow->refreshShop();
                } });

    // 商店关闭回调
    connect(m_shopWindow, &ShopWindow::shopClosed, this, [this, mainWindow, onClose]()
            {
                checkAndMergeStars();
                if (mainWindow && onClose)
                {
                    (mainWindow->*onClose)(MainWindow::RefreshAll);
                } });
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
                          .arg(units[target].getUuid()));
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
        if (units[i].getUuid() == uuid)
        {
            int star = units[i].starLevel;
            int cost = units[i].cost;
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
    return m_tower ? static_cast<int>(m_tower->currentHp) : 0;
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
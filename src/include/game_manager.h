#ifndef GAMEMANAGER_H
#define GAMEMANAGER_H

#include <QObject>
#include <QTimer>
#include <QRandomGenerator>
#include <vector>
#include "state.h"

#define GAME_TICK_INTERVAL_MS 50
#define BASE_TOWER_HP 10000
class DatabaseManager;

class GameManager : public QObject
{
    Q_OBJECT

public:
    explicit GameManager(QObject *parent = nullptr);
    ~GameManager();

    // 初始化并重置对局数据
    void initialize();

    // 启动一轮战斗（默认第 1 轮）
    void startRound(int roundNumber = 1);
    void stopRound();

    /// 进入下一轮准备阶段（ShowResult → Finish → Prepare）
    void nextRound();

    RoundPhase getCurrentPhase() const { return m_phase; }
    int getRoundNumber() const { return m_roundNumber; }
    int getTowerHp() const { return m_towerHp; }
    int getMaxTowerHp() const { return m_maxTowerHp; }
    PlayerAssets &getPlayerAssets() { return m_player; }
    const PlayerAssets &getPlayerAssets() const { return m_player; }
    const std::vector<EnemyInstance> &getEnemies() const { return m_enemies; }

    /// 设置/获取本局随机种子
    void setGameSeed(quint32 seed)
    {
        m_gameSeed = seed;
        m_rng.seed(seed);
    }
    quint32 gameSeed() const { return m_gameSeed; }

    /// 设置图鉴数据库引用
    void setDatabase(DatabaseManager *db) { m_database = db; }

    /// 检查并执行升星合并（3个同种同星→1个高星）
    void checkAndMergeStars();

    /// 出售单位，返回获得的费用
    int sellUnit(int uuid);

    /// 本回合获得的金币/经验（与回合开始前快照对比）
    int getRoundGoldEarned() const { return m_player.gold - m_roundStartGold; }
    int getRoundExpEarned() const { return m_player.exp - m_roundStartExp; }

    /// 本回合学分值（第1回合2分，第2回合3分，第3回合5分）
    static int getRoundCredit(int round)
    {
        switch (round)
        {
        case 1:
            return 2;
        case 2:
            return 3;
        case 3:
            return 5;
        default:
            return 0;
        }
    }
    int getMaxRounds() const { return m_maxRounds; }

    /// 本回合学分绩 = (塔血% × 4.0) 满分4.0
    double getRoundGpa() const
    {
        if (m_maxTowerHp <= 0)
            return 0.0;
        double ratio = static_cast<double>(m_towerHp) / static_cast<double>(m_maxTowerHp);
        if (ratio < 0.0)
            ratio = 0.0;
        return ratio * 4.0;
    }
    /// 已完成回合的加权平均学分绩
    double getAverageGpa() const
    {
        return m_totalCredits > 0 ? m_weightedGpaSum / m_totalCredits : 0.0;
    }
    /// 已完成回合的学分之和
    int getPreviousCredits() const { return m_totalCredits; }

signals:
    // 每次 tick 发出，用于驱动 UI 刷新
    void tick();
    // 回合阶段变化
    void phaseChanged(RoundPhase newPhase);
    // 回合结束，参数为是否我方胜利
    void roundEnded(bool victory);
    /// 伤害/治疗跳字 (文字, 场景x, 场景y, #RRGGBB颜色)
    void floatingText(const QString &text, double x, double y, const QString &color);
    /// 游戏结束（三回合打完）
    void gameOver(double finalGpa, int totalGold, int totalExp);

private slots:
    void onTick();

private:
    void transitionPhase(RoundPhase newPhase);
    void spawnEnemies(int roundNumber);
    std::vector<EnemyConfig> pickRandomEnemies(int count, int roundNumber);
    void executeAttackCycle(double deltaSeconds);
    bool checkCombatEndConditions(bool &outVictory);
    void resetUnitsForNextRound();

private:
    QTimer *m_tickTimer;
    int m_tickIntervalMs = GAME_TICK_INTERVAL_MS;
    int m_roundNumber;
    RoundPhase m_phase;

    PlayerAssets m_player;
    std::vector<EnemyInstance> m_enemies;
    std::vector<EnemyConfig> m_enemyConfigs; // storage for configs lifetime

    int m_towerHp;
    int m_maxTowerHp = BASE_TOWER_HP;
    double m_towerHpMultiplier = 1.0;
    int m_roundStartGold = 0;
    int m_roundStartExp = 0;
    int m_guaranteedGold = 10;
    int m_pendingGold = 0; // 战斗中获得的临时金币
    int m_pendingExp = 0;  // 战斗中获得的临时经验
    int m_maxRounds = 3;
    double m_weightedGpaSum = 0.0; // Σ(学分×学分绩)
    int m_totalCredits = 0;        // 已完成回合的学分之和

    double m_towerAttackCooldown = 0.0; // 塔攻击冷却
    double m_timeAccumulator;
    quint32 m_gameSeed = 12345;
    QRandomGenerator m_rng{12345};
    DatabaseManager *m_database = nullptr;
};

#endif
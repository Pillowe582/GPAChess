#ifndef GAMEMANAGER_H
#define GAMEMANAGER_H

#include <QObject>
#include <QTimer>
#include <vector>
#include <unordered_map>

#include "state.h"

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

    /// 本回合获得的金币/经验（与回合开始前快照对比）
    int getRoundGoldEarned() const { return m_player.gold - m_roundStartGold; }
    int getRoundExpEarned() const { return m_player.exp - m_roundStartExp; }

    /// 本回合学分绩 = (剩余塔血 / 最大塔血) × 4.0（模拟GPA）
    double getRoundGpa() const
    {
        if (m_maxTowerHp <= 0)
            return 0.0;
        double ratio = static_cast<double>(m_towerHp) / static_cast<double>(m_maxTowerHp);
        if (ratio < 0.0)
            ratio = 0.0;
        return ratio * 4.0;
    }

signals:
    // 每次 tick 发出，用于驱动 UI 刷新
    void tick();
    // 回合阶段变化
    void phaseChanged(RoundPhase newPhase);
    // 回合结束，参数为是否我方胜利
    void roundEnded(bool victory);
    /// 伤害/治疗跳字 (文字, 场景x, 场景y, R, G, B)
    void floatingText(const QString &text, double x, double y, int r, int g, int b);

private slots:
    void onTick();

private:
    void transitionPhase(RoundPhase newPhase);
    void spawnEnemies(int roundNumber);
    void executeAttackCycle(double deltaSeconds);
    bool checkCombatEndConditions(bool &outVictory);
    void resetUnitsForNextRound();

private:
    QTimer *m_tickTimer;
    int m_tickIntervalMs;
    int m_roundNumber;
    RoundPhase m_phase;

    PlayerAssets m_player;
    std::vector<EnemyInstance> m_enemies;
    std::vector<EnemyConfig> m_enemyConfigs; // storage for configs lifetime

    int m_towerHp;
    int m_maxTowerHp = 100;
    int m_roundStartGold = 0;
    int m_roundStartExp = 0;

    // 每 uuid 的攻击冷却剩余（秒）
    std::unordered_map<int, double> m_attackCooldownRemaining;
    double m_towerAttackCooldown = 0.0; // 塔攻击冷却
    double m_timeAccumulator;
};

#endif
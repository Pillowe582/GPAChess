#ifndef GAMEMANAGER_H
#define GAMEMANAGER_H

#include <QObject>
#include "mainwindow.h"
#include "shop_window.h"
#include <QTimer>
#include <QRandomGenerator>
#include <vector>
#include "state.h"

#define GAME_TICK_INTERVAL_MS 5
#define BASE_TOWER_HP 10000
class DatabaseManager;
class Renderer;
class TowerBehavior;

#include <memory> // for unique_ptr

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

    // % Getters
    RoundPhase getCurrentPhase() const { return m_phase; }
    int getRoundNumber() const { return m_roundNumber; }
    int getTowerHp() const;
    int getMaxTowerHp() const;
    PlayerAssets &getPlayerAssets() { return m_player; }
    const PlayerAssets &getPlayerAssets() const { return m_player; }
    const std::vector<EnemyInstance> &getEnemies() const { return m_enemies; }
    Renderer *getRenderer() const { return m_renderer; }

    const std::vector<RoundInfo> &getRoundInfos() const { return m_roundInfos; }

    /// 本回合获得的金币/经验（与回合开始前快照对比）
    int getRoundGoldEarned() const { return m_player.gold - m_roundStartGold; }
    int getRoundExpEarned() const { return m_player.exp - m_roundStartExp; }

    // % Setters

    /// 设置渲染器引用（由 MainWindow 注入）
    void setRenderer(Renderer *r) { m_renderer = r; }

    quint32 gameSeed() const { return m_gameSeed; }

    // % 工具方法
    /// 检查并执行升星合并（3个同种同星→1个高星）
    void checkAndMergeStars();

    // 打开商店，以及商店关闭时的回调
    void openShop(MainWindow *mainWindow, void (MainWindow::*onClose)(int));
    void openShop() // 无回调版本
    {
        openShop(nullptr, nullptr);
    }

    // 计算单位总价值（用于出售时返还金币）
    int getTotalWorth(int star, int cost)
    {
        int total = cost;
        for (int s = 1; s < star; ++s)
            total *= 3;
        return total;
    }

    /// 出售单位，返回获得的费用
    int sellUnit(int uuid);

    /// 本回合学分值（随机）
    int generateRoundCredit(int round);
    int getMaxRounds() const { return m_maxRounds; }

    // 本回合学分绩 = (塔血百分比 × 4.0) 满分4.0
    double getRoundGpa() const
    {
        if (!m_tower)
            return 0.0;
        double ratio = m_tower->maxHp.getFinal() > 0
                           ? m_tower->currentHp / m_tower->maxHp.getFinal()
                           : 0.0;
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
    void splashText(const QString &text, double x, double y, const QString &color);
    /// 游戏结束
    void gameOver(double finalGpa);
    /// 实体受到伤害（转发生 LivingEntity 的信号）
    void receivedDamage(LivingEntity *entity, double amount, QColor color);

private slots:
    void onTick();

private:
    void transitionPhase(RoundPhase newPhase);
    void spawnEnemies(int roundNumber, int count, bool mandatory = true);
    std::vector<EnemyConfig> pickRandomEnemies(int roundNumber, int count, bool mandatory = true);
    void tickBehaviors(double deltaSeconds);
    bool checkCombatEndConditions(bool &outVictory);
    void resetUnitsForNextRound();
    void generateRoundInfos(std::vector<RoundInfo> &out, int maxRounds); // 生成每轮的学分值和敌人数量

private:
    QTimer *m_tickTimer;
    int m_roundNumber;

    // 当前回合阶段
    RoundPhase m_phase;

    PlayerAssets m_player;
    std::vector<EnemyInstance> m_enemies;
    std::vector<EnemyConfig> m_enemyConfigs;
    std::vector<RoundInfo> m_roundInfos;

    ChessInstance *m_tower = nullptr; // 指向 ownedChesses 中的塔实例
    double m_towerHpMultiplier = 1.0;
    int m_roundStartGold = 0;
    int m_roundStartExp = 0;
    int m_guaranteedGold = 10;
    int m_pendingGold = 0; // 战斗中获得的临时金币
    int m_pendingExp = 0;  // 战斗中获得的临时经验

    // Elective 敌人生成追踪
    int m_electiveEnemiesNotSpawned = 0;    // 未生成的 elective 敌人数量
    bool m_mandatoryEnemiesCleared = false; // 必修敌人是否已清除

    // 最大回合数
    int m_maxRounds = 10;

    double m_weightedGpaSum = 0.0; // Σ(学分×学分绩)
    int m_totalCredits = 0;        // 已完成回合的学分之和

    double m_timeAccumulator;
    quint32 m_gameSeed = 12345;
    ShopWindow *m_shopWindow = nullptr;
    QRandomGenerator m_rng{12345};
    DatabaseManager *m_database = nullptr;
    Renderer *m_renderer = nullptr; // 渲染器（由 MainWindow 注入）
};

#endif
#ifndef GAMEMANAGER_H
#define GAMEMANAGER_H

#include <QObject>
#include <QTimer>
#include <QRandomGenerator>
#include <vector>
#include "state.h"

#define GAME_TICK_INTERVAL_MS 5
#define BASE_TOWER_HP 10000

// 前向声明
class DatabaseManager;
class Renderer;
class TowerBehavior;
class ShopWindow;
class RegistrationWindow;
class MainWindow;

#include <memory> // for unique_ptr
#include "state.h"

// 游戏实体集合，提供导航接口
struct GameEntities
{
    std::vector<AllyInstance *> allies;
    std::vector<EnemyInstance *> enemies;
    std::vector<BaseEntity *> neutral; // 预留给中立单位（如陷阱、障碍物等）

    void startBehaviors()
    {
        for (const auto &ally : this->allies)
        {
            if (ally && ally->behavior)
                ally->behavior->onStart(*ally);
        }
        for (const auto &enemy : this->enemies)
        {
            if (enemy && enemy->behavior)
                enemy->behavior->onStart(*enemy);
        }
    }
};

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
    const GameEntities &getGameEntities() const { return m_gameEntities; }
    GameEntities &getGameEntities() { return m_gameEntities; }
    Renderer &getRenderer() const { return *m_renderer; }

    const std::vector<RoundInfo> &getRoundInfos() const { return m_roundInfos; }

    std::vector<RoundInfo> &getRoundInfos() { return m_roundInfos; };

    const int getMaxRounds() const { return m_maxRounds; }
    int &getMaxRounds() { return m_maxRounds; }

    /// 本回合获得的金币/经验（与回合开始前快照对比）
    int getRoundGoldEarned() const { return m_pendingGold; }
    int getRoundExpEarned() const { return m_pendingExp; }

    // % Setters

    /// 设置渲染器引用（由 MainWindow 注入）
    void setRenderer(Renderer *r) { m_renderer = r; }

    void addPendingRewards(int gold = 0, int exp = 0)
    {
        m_pendingGold += gold;
        m_pendingExp += exp;
    }

    QTimer *getTickTimer() const { return m_tickTimer; }
    quint32 gameSeed() const { return m_gameSeed; }

    // % 工具方法
    /// 限制实体位置在画布内（保留 margin 边距）
    void clampToArena(BaseEntity &entity, double margin = 100.0) const;
    /// 检查并执行升星合并（3个同种同星→1个高星）
    void checkAndMergeStars();
    void applySeparation(std::vector<EnemyInstance *> &enemies,
                         std::vector<std::unique_ptr<AllyInstance>> &allies,
                         double dt);

    // 打开商店，以及商店关闭时的回调
    void openShop(MainWindow *mainWindow, void (MainWindow::*onClose)(int));
    void openShop() // 无回调版本
    {
        openShop(nullptr, nullptr);
    }
    // 生成轮次信息
    void generateRoundInfos(std::vector<RoundInfo> &out, int maxRounds);

    // 打开选课界面
    void showRegistrationWindow(MainWindow *mainWindow, GameManager *gm);

    // 检查必修敌人是否已清除
    bool isMandatoryEnemiesCleared() const { return m_mandatoryEnemiesCleared; }

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

    // 等级计算：从第 n 级升到 n+1 级需要 10 * log2(n+2) 经验
    static int expForLevel(int level)
    {
        if (level < 0)
            level = 0;
        return static_cast<int>(10.0 * std::log2(level + 2.0));
    }

    /// 根据累计 exp 计算当前等级
    static int levelFromExp(int exp)
    {
        int lv = 0;
        while (true)
        {
            int needed = expForLevel(lv);
            if (exp < needed)
                break;
            exp -= needed;
            ++lv;
        }
        return lv;
    }

    /// 当前等级下的经验进度（当前等级已获得的经验）
    static int expInCurrentLevel(int totalExp)
    {
        int lv = 0;
        while (true)
        {
            int needed = expForLevel(lv);
            if (totalExp < needed)
                break;
            totalExp -= needed;
            ++lv;
        }
        return totalExp;
    }

    /// 当前等级升到下一级所需的总经验
    static int expNeededForNextLevel(int totalExp)
    {
        int lv = levelFromExp(totalExp);
        return expForLevel(lv);
    }

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
    std::vector<EnemyConfig> pickRandomEnemies(int roundNumber, int count);
    void tickBehaviors(double deltaSeconds);
    bool checkCombatEndConditions(bool &outVictory);
    void resetUnitsForNextRound();

private:
    QTimer *m_tickTimer;
    int m_roundNumber;

    // 当前回合阶段
    RoundPhase m_phase;

    PlayerAssets m_player;
    GameEntities m_gameEntities;

    std::vector<EnemyConfig> m_enemyConfigs;
    std::vector<RoundInfo> m_roundInfos;

    AllyInstance *m_tower = nullptr; // 指向 ownedChesses 中的塔实例
    double m_towerHpMultiplier = 1.0;
    int m_roundStartGold = 0;
    int m_roundStartExp = 0;
    int m_guaranteedGold = 10;
    int m_pendingGold = 0; // 战斗中获得的临时金币
    int m_pendingExp = 0;  // 战斗中获得的临时经验

    // Elective 敌人生成追踪
    int m_electiveEnemiesNotSpawned = 0;    // 未生成的 elective 敌人数量
    int m_electiveEnemiesWave = 0;          // 当前 elective 敌人波次
    bool m_mandatoryEnemiesCleared = false; // 必修敌人是否已清除

    // 最大回合数
    int m_maxRounds = 10;

    double m_weightedGpaSum = 0.0; // Σ(学分×学分绩)
    int m_totalCredits = 0;        // 已完成回合的学分之和

    double m_timeAccumulator;
    quint32 m_gameSeed = 12345;
    ShopWindow *m_shopWindow = nullptr;
    RegistrationWindow *m_registrationWindow = nullptr;
    QRandomGenerator m_rng{12345};
    DatabaseManager *m_database = nullptr;
    Renderer *m_renderer = nullptr; // 渲染器（由 MainWindow 注入）
};

#endif
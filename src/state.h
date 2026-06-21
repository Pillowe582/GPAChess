#ifndef GAMESTATE_H
#define GAMESTATE_H

#include <QString>
#include <QColor>
#include <QStringList>
#include <vector>
#include <atomic>
#include <memory>

class GameManager;
class Renderer;

#include "entity/ally_behavior.h"
#include "entity/enemy_behavior.h"

// ============================================================================
// % 基本数据定义
// ============================================================================

enum class RoundPhase
{
    Prepare,    // 1. 准备阶段
    Calculate,  // 2. 系统计算阶段
    Combat,     // 3. 对战阶段
    ShowResult, // 4. 结算界面
    Finish      // 5. 结束阶段
};

struct Transform
{
    double x = 0;
    double y = 0;
    double rot = 0;
    double scaleX = 1.0;
    double scaleY = 1.0;
    void reset()
    {
        x = y = rot = 0;
        scaleX = scaleY = 1.0;
    }
};

struct DamageType
{
    enum Type
    {
        Physical,
        True
    };
    Type type = Physical;
    QColor color;
};

class Attribute
{
public:
    double base = 0;
    double flatBonus = 0;
    float percentBonus = 0.0f;
    double getFinal() const { return (base + flatBonus) * (1.0f + percentBonus); }
};

// ============================================================================
// % 静态图鉴配置
// ============================================================================

class BaseConfig
{
public:
    int configId = 0;
    QString name;
    int baseHp = 0, baseAtk = 0, baseDef = 0;
    float hpGrowthMultiplier = 1.0f, atkGrowthMultiplier = 1.0f;
    int attackRange = 1;
    float baseAttackSpeed = 1;
    int maxMp = 0;
    double speed = 1.0;
    int behaviorId = 1;
    QString description;
};

class AllyConfig : public BaseConfig
{
public:
    int cost = 1;
    QStringList bonds;
};

class EnemyConfig : public BaseConfig
{
public:
    int baseGoldReward = 0;
    int baseExpReward = 0;
};

// ============================================================================
// % 运行时实体（Unity 风格层级）
// ============================================================================

/// 所有场景物体的基类 —— 类似 Unity GameObject
class BaseEntity
{
public:
    BaseEntity(GameManager *mgr = nullptr) : gameManager(mgr) {}
    virtual ~BaseEntity() = default;

    int layer = 0;
    GameManager *gameManager = nullptr;
    Transform transform;
};

/// 有生命的实体 —— 类似 Unity 中带 Health 组件的 GameObject
class LivingEntity : public BaseEntity
{
public:
    LivingEntity(GameManager *mgr = nullptr) : BaseEntity(mgr)
    {
        static std::atomic<int> s_counter{1};
        uuid = s_counter.fetch_add(1);
        currentHp = maxHp.getFinal();
        currentDef = maxDef.getFinal();
        currentSpeed = maxSpeed.getFinal();
    }

    // ---- 标识 ----
    int uuid = 0;
    int getUuid() const { return uuid; }

    // ---- 属性 ----
    Attribute maxHp;
    Attribute maxDef;
    Attribute maxSpeed;
    double currentHp = 0.0;
    double currentDef = 0.0;
    double currentSpeed = 0.0;
    bool isAlive = true;

    // ---- getters ----
    double getMaxHp() const { return maxHp.getFinal(); }
    double getCurrentHp() const { return currentHp; }
    double getCurrentDef() const { return currentDef; }

    // ---- 便捷 Transform 访问 ----
    double getX() const { return transform.x; }
    double getY() const { return transform.y; }
    void setX(double v) { transform.x = v; }
    void setY(double v) { transform.y = v; }
    void setPosition(double x, double y)
    {
        transform.x = x;
        transform.y = y;
    }

    // ---- 生命 ----
    virtual void setDeath()
    {
        isAlive = false;
        currentHp = 0;
    }

    bool stillAlive() const { return isAlive; }

    void takeDamage(double rawDamage)
    {
        dealDamage(rawDamage, {DamageType::Physical, QColor("#FFFFFF")});
    }

    double dealDamage(double rawDamage, DamageType type)
    {
        if (!isAlive)
            return 0.0;
        double actual = rawDamage;
        if (type.type == DamageType::Physical)
            actual *= 100.0 / (100.0 + currentDef);
        currentHp -= actual;
        if (currentHp <= 0)
            setDeath();
        return actual;
    }
};

// ============================================================================
// % 运行时实例
// ============================================================================

class ChessInstance : public AllyConfig, public LivingEntity
{
public:
    explicit ChessInstance(const AllyConfig &cfg, GameManager *mgr = nullptr)
        : AllyConfig(cfg), LivingEntity(mgr)
    {
        calculateBaseStatsByStar();
        resetStatus();
    }

    int starLevel = 1;
    Attribute atk;
    int currentMp = 0;
    bool deployed = false;
    int benchSlot = -1;

    double savedPosX = 0, savedPosY = 0;
    int targetEnemyId = -1;

    std::unique_ptr<AllyBehavior> behavior;

    void calculateBaseStatsByStar()
    {
        for (int i = 1; i < starLevel; ++i)
        {
            maxHp.base *= hpGrowthMultiplier;
            atk.base *= atkGrowthMultiplier;
        }
    }

    void resetStatus()
    {
        currentHp = maxHp.getFinal();
        currentMp = 0;
        currentDef = maxDef.getFinal();
        currentSpeed = maxSpeed.getFinal();
        isAlive = true;
        targetEnemyId = -1;
    }
};

class EnemyInstance : public EnemyConfig, public LivingEntity
{
public:
    EnemyInstance(const EnemyConfig &cfg, bool required, int round,
                  GameManager *mgr = nullptr)
        : EnemyConfig(cfg), LivingEntity(mgr), isRequired(required)
    {
        maxHp.base *= 1.0 + (round - 1) * hpGrowthMultiplier;
        atk.base *= 1.0 + (round - 1) * atkGrowthMultiplier;
        currentHp = maxHp.getFinal();
        isAlive = true;
    }

    Attribute atk;
    bool isRequired = false;
    int currentTargetId = -1;

    std::unique_ptr<EnemyBehavior> behavior;
};

// ============================================================================
// % 玩家数据
// ============================================================================

class PlayerAssets
{
public:
    static constexpr int maxBattlefield = 5;
    static constexpr int maxBench = 11;

    int gold = 0, exp = 0;
    int totalCredit = 0, totalGradePoint = 0;
    std::vector<ChessInstance> ownedChesses;

    int deployedCount() const
    {
        int n = 0;
        for (auto &c : ownedChesses)
            if (c.deployed)
                ++n;
        return n;
    }
    int benchCount() const
    {
        int n = 0;
        for (auto &c : ownedChesses)
            if (!c.deployed)
                ++n;
        return n;
    }
    bool battlefieldFull() const { return deployedCount() >= maxBattlefield; }
    bool benchFull() const { return benchCount() >= maxBench; }

    void compactBenchSlots()
    {
        int next = 0;
        for (auto &c : ownedChesses)
            if (!c.deployed)
                c.benchSlot = next++;
    }

    int firstEmptyBenchSlot() const
    {
        bool occupied[maxBench] = {};
        for (auto &c : ownedChesses)
            if (!c.deployed && c.benchSlot >= 0 && c.benchSlot < maxBench)
                occupied[c.benchSlot] = true;
        for (int i = 0; i < maxBench; ++i)
            if (!occupied[i])
                return i;
        return -1;
    }

    ChessInstance *getUnitByUuid(int uuid)
    {
        for (auto &u : ownedChesses)
            if (u.uuid == uuid)
                return &u;
        return nullptr;
    }
    const ChessInstance *getUnitByUuid(int uuid) const
    {
        for (auto &u : ownedChesses)
            if (u.uuid == uuid)
                return &u;
        return nullptr;
    }
};

#endif

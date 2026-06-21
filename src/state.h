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
    void reset();
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
    double getFinal() const;
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
// % 运行时实体
// ============================================================================

/// 所有场景物体的基类
class BaseEntity
{
public:
    BaseEntity(GameManager *mgr = nullptr) : gameManager(mgr) {}
    virtual ~BaseEntity() = default;

    int layer = 0;
    GameManager *gameManager = nullptr;
    Transform transform;
};

/// 有生命的实体基类
class LivingEntity : public BaseEntity
{
public:
    LivingEntity(GameManager *mgr = nullptr);

    // ---- 标识 ----
    int uuid = 0;
    int getUuid() const;

    // ---- 属性 ----
    Attribute maxHp;
    Attribute maxDef;
    Attribute maxSpeed;
    double currentHp = 0.0;
    double currentDef = 0.0;
    double currentSpeed = 0.0;
    bool isAlive = true;

    // ---- getters ----
    double getMaxHp() const;
    double getCurrentHp() const;
    double getCurrentDef() const;

    // ---- 便捷 Transform 访问 ----
    double getX() const;
    double getY() const;
    void setX(double v);
    void setY(double v);
    void setPosition(double x, double y);

    // ---- 生命 ----
    virtual void setDeath();

    bool stillAlive() const;

    // ---- 造成伤害 ----
    double dealDamage(double rawDamage); // 默认为物理伤害
    double dealDamage(double rawDamage, DamageType type);
};

// ============================================================================
// % 运行时实例
// ============================================================================

class ChessInstance : public AllyConfig, public LivingEntity
{
public:
    explicit ChessInstance(const AllyConfig &cfg, GameManager *mgr = nullptr);

    int starLevel = 1;
    Attribute atk;
    int currentMp = 0;
    bool deployed = false;
    bool isTower = false; // 塔标记：不占部署位，不可拖拽
    int benchSlot = -1;

    double savedPosX = 0, savedPosY = 0;
    int targetEnemyId = -1;

    std::unique_ptr<AllyBehavior> behavior;

    void calculateBaseStatsByStar();
    void resetStatus();
};

class EnemyInstance : public EnemyConfig, public LivingEntity
{
public:
    EnemyInstance(const EnemyConfig &cfg, bool required, int round,
                  GameManager *mgr = nullptr);

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

    int deployedCount() const;
    int benchCount() const;
    bool battlefieldFull() const;
    bool benchFull() const;

    void compactBenchSlots();

    int firstEmptyBenchSlot() const;

    ChessInstance *getUnitByUuid(int uuid);
    const ChessInstance *getUnitByUuid(int uuid) const;
};

#endif

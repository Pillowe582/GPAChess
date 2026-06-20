#ifndef GAMESTATE_H
#define GAMESTATE_H

#include <QString>
#include <QStringList>
#include <vector>
#include <atomic>
#include <memory>

#include "entity/ally_behavior.h"
#include "entity/enemy_behavior.h"

/// 渲染指令（纯数据，无逻辑）
struct DrawCmd
{
    enum Kind
    {
        Circle,     // 实心圆（子弹等）
        RotatedRect // 旋转矩形（武器挥砍等）
    };
    Kind kind = Circle;
    double x = 0, y = 0;
    double param1 = 0; // Circle: radius, RotatedRect: half-width
    double param2 = 0; // RotatedRect: half-height
    double angle = 0;
    QColor color;
    int zValue = 100;
};

// 状态机阶段枚举
enum class RoundPhase
{
    Prepare,    // 1. 准备阶段
    Calculate,  // 2. 系统计算阶段
    Combat,     // 3. 对战阶段
    ShowResult, // 4. 结算界面
    Finish      // 5. 结束阶段
};

// 统一的属性容器（用于运行时计算）
class Attribute
{
public:
    int base = 0;              // 白值
    int flatBonus = 0;         // 小加成
    float percentBonus = 0.0f; // 百分比加成

    // 运行时动态计算最终属性
    int getFinal() const
    {
        return static_cast<int>((base + flatBonus) * (1.0f + percentBonus));
    }
};

inline void applyDamageToUnit(int rawDamage, const Attribute &def, int &currentHp, bool &isAlive)
{
    if (!isAlive)
        return;

    int finalDef = def.getFinal();
    int actualDamage = rawDamage * 100 / (100 + finalDef);
    if (actualDamage < 1)
        actualDamage = 1;

    currentHp -= actualDamage;
    if (currentHp <= 0)
    {
        currentHp = 0;
        isAlive = false;
    }
}

// 基础图鉴配置：包含双方（棋子/敌人）共有的基础数值
class BaseConfig
{
public:
    int configId = 0; // 种类ID
    QString name;     // 名字

    // 1星基础三维属性
    int baseHp = 0;
    int baseAtk = 0;
    int baseDef = 0;

    // 属性成长的倍率系数
    float hpGrowthMultiplier = 1.0f;
    float atkGrowthMultiplier = 1.0f;

    int attackRange = 1;       // 攻击距离
    float baseAttackSpeed = 1; // 基础攻速
    int maxMp = 0;             // 满蓝条上限
    double speed = 1.0;        // 移动速度
    int behaviorId = 1;        // 行为ID

    QString description;
};

// 静态配置：玩家棋子静态图鉴
class ChessConfig : public BaseConfig
{
public:
    int cost = 1;      // 几费
    QStringList bonds; // 羁绊标签
};

// 静态配置：敌人图鉴
class EnemyConfig : public BaseConfig
{
public:
    int baseGoldReward = 0;           // 击杀奖励金币
    int baseExpReward = 0;            // 击杀奖励经验
    bool isRequiredByDefault = false; // 是否为必修单位
};

// 运行时角色实例：继承对应的静态配置，实例化后拥有静态数值与动态状态
class ChessInstance : public ChessConfig
{
public:
    // Constructor with explicit UUID
    explicit ChessInstance(int uId, const ChessConfig &staticConfig)
        : ChessConfig(staticConfig), uuid(uId)
    {
        calculateBaseStatsByStar();
        resetStatus();
    }

    // Constructor that auto-assigns a unique UUID
    explicit ChessInstance(const ChessConfig &staticConfig)
        : ChessConfig(staticConfig)
    {
        static std::atomic<int> s_counter{1}; // 定义一个静态原子计数器，用于生成唯一的UUID
        uuid = s_counter.fetch_add(1);        // 每次构造时递增计数器并返回旧值作为UUID
        calculateBaseStatsByStar();
        resetStatus();
    }

    // 实例元数据
    int uuid = 0; // 局内自增id

    // 运行时数值
    int starLevel = 1;     // 当前星级
    Attribute hp;          // 最终生命值属性
    Attribute atk;         // 最终攻击力属性
    Attribute def;         // 最终防御力属性
    Attribute attackSpeed; // 最终攻击速度

    // 局内实时状态
    int currentHp = 0;   // 当前实时剩余血量
    int currentMp = 0;   // 当前实时能量/蓝量
    bool isAlive = true; // 当前是否存活

    // 部署与备战状态
    bool deployed = false; // true=在战场上, false=在备战席
    int benchSlot = -1;    // 备战席槽位索引 0~10, 仅在 !deployed 时有效; -1 表示未分配

    // 位置与 AI 状态
    double posX = 0;        // 战场上的 X 坐标 (仅在 deployed 时有效)
    double posY = 0;        // 战场上的 Y 坐标 (仅在 deployed 时有效)
    int targetEnemyId = -1; // 当前索敌的UUID，-1表示没有目标

    std::unique_ptr<AllyBehavior> behavior; // 攻击行为策略

    void calculateBaseStatsByStar()
    {
        if (starLevel > 1)
        {
            hp.base = baseHp;
            atk.base = baseAtk;
            def.base = baseDef;

            for (int i = 1; i < starLevel; ++i)
            {
                hp.base = static_cast<int>(hp.base * hpGrowthMultiplier);
                atk.base = static_cast<int>(atk.base * atkGrowthMultiplier);
            }
        }
        else
        {
            hp.base = baseHp;
            atk.base = baseAtk;
            def.base = baseDef;
        }
        attackSpeed.base = static_cast<int>(baseAttackSpeed);
    }

    void resetStatus() // 用于每轮开始时重置棋子状态
    {
        currentHp = hp.getFinal();
        currentMp = 0;
        isAlive = true;
        targetEnemyId = -1;
    }

    void takeDamage(int rawDamage)
    {
        applyDamageToUnit(rawDamage, def, currentHp, isAlive);
    }
};

// 运行时敌人实例
class EnemyInstance : public EnemyConfig
{
public:
    EnemyInstance(int uId, const EnemyConfig &staticConfig, bool isRequired, int currentRound)
        : EnemyConfig(staticConfig), uuid(uId), isRequired(isRequired)
    {
        hp.base = baseHp;
        hp.percentBonus = (currentRound - 1) * 0.15f;

        atk.base = baseAtk;
        atk.percentBonus = (currentRound - 1) * 0.10f;

        def.base = baseDef;
        attackSpeed.base = static_cast<int>(baseAttackSpeed);

        currentHp = hp.getFinal();
        isAlive = true;
        currentTargetId = -1;
    }

    int uuid = 0;
    Attribute hp;
    Attribute atk;
    Attribute def;
    Attribute attackSpeed;

    int currentHp = 0;
    bool isAlive = true;

    bool isRequired = false;
    int currentTargetId = -1;

    double posX = 0;
    double posY = 0;

    std::unique_ptr<EnemyBehavior> behavior;

    void takeDamage(int rawDamage)
    {
        applyDamageToUnit(rawDamage, def, currentHp, isAlive);
    }
};

// 玩家全局对局数据
class PlayerAssets
{
public:
    static constexpr int maxBattlefield = 5; // 战场上最多可部署单位数
    static constexpr int maxBench = 11;      // 备战席最多可容纳单位数

    int gold = 0;
    int exp = 0;
    int totalCredit = 0;                     // 累计已修学分
    int totalGradePoint = 0;                 // 累计学分绩
    std::vector<ChessInstance> ownedChesses; // 玩家拥有的所有棋子

    // ---- 容量查询 ----
    int deployedCount() const
    {
        int n = 0;
        for (const auto &c : ownedChesses)
            if (c.deployed)
                ++n;
        return n;
    }
    int benchCount() const
    {
        int n = 0;
        for (const auto &c : ownedChesses)
            if (!c.deployed)
                ++n;
        return n;
    }
    bool battlefieldFull() const { return deployedCount() >= maxBattlefield; }
    bool benchFull() const { return benchCount() >= maxBench; }

    // ---- 备战席槽位压缩（移除空槽、重新编号） ----
    void compactBenchSlots()
    {
        int nextSlot = 0;
        for (auto &c : ownedChesses)
        {
            if (!c.deployed)
                c.benchSlot = nextSlot++;
        }
    }

    // ---- 查找第一个空备战槽位 ----
    int firstEmptyBenchSlot() const
    {
        // brute-force: benchmark the occupied slots
        bool occupied[maxBench] = {};
        for (const auto &c : ownedChesses)
            if (!c.deployed && c.benchSlot >= 0 && c.benchSlot < maxBench)
                occupied[c.benchSlot] = true;
        for (int i = 0; i < maxBench; ++i)
            if (!occupied[i])
                return i;
        return -1; // 备战席已满
    }
};

#endif
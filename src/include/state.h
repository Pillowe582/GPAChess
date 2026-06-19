#ifndef GAMESTATE_H
#define GAMESTATE_H

#include <QString>
#include <QStringList>
#include <vector>

// 状态机阶段枚举
enum class RoundPhase
{
    Prepare,    // 1. 准备阶段
    Calculate,  // 2. 系统计算阶段
    Combat,     // 3. 对战阶段
    ShowResult, // 4. 结算界面
    Finish      // 5. 结束阶段
};

struct Attribute
{
    int base = 0;              // 白值
    int flatBonus = 0;         // 小加成
    float percentBonus = 0.0f; // 百分比加成

    // 运行时动态计算最终属性
    int getFinal() const
    {
        return static_cast<int>((base + flatBonus) * (1.0f + percentBonus));
    }
};

// 静态配置：角色的初始图鉴数值
struct ChessConfig
{
    int configId; // 种类ID
    QString name; // 名字
    int cost;     // 几费

    // 1星基础三维属性
    int baseHp;
    int baseAtk;
    int baseDef;

    // 属性成长的倍率系数
    float hpGrowthMultiplier;
    float atkGrowthMultiplier;

    int attackRange;       // 攻击距离
    float baseAttackSpeed; // 基础攻速
    int maxMp;             // 满蓝条上限
    double speed;          // 移动速度

    QStringList bonds;   // 拥有的初始羁绊标签
    QString description; // 技能或角色描述
};

// 运行时角色实例：战场上每一个真实存在的棋子
class ChessInstance
{
public:
    // ==========================================
    // 1. 实例元数据
    // ==========================================
    int uuid;                 // 局内自增id
    const ChessConfig *config; // 指向对应静态配置图鉴

    // ==========================================
    // 2. 动态生成的复合战斗属性
    // ==========================================
    int starLevel = 1;     // 当前星级（1星、2星、3星）
    Attribute hp;          // 最终生命值属性
    Attribute atk;         // 最终攻击力属性
    Attribute def;         // 最终防御力属性
    Attribute attackSpeed; // 最终攻击速度

    // ==========================================
    // 3. 局内对战实时状态
    // ==========================================
    int currentHp = 0;   // 当前实时剩余血量
    int currentMp = 0;   // 当前实时能量/蓝量
    bool isAlive = true; // 当前是否存活

    // ==========================================
    // 4. 位置与空间状态
    // ==========================================
    double posX = 0;
    double posY = 0;

    // 战斗 AI 专用目标锁
    int targetEnemyId = -1; // 当前索敌的UUID，-1表示没有目标

public:
    // 构造函数：必须传入一个静态配置来孵化这个实例
    explicit ChessInstance(int uId, const ChessConfig *staticConfig)
        : uuid(uId), config(staticConfig)
    {
        calculateBaseStatsByStar(); // 依据星级计算出最初的 base 属性
        resetStatus();              // 将实时血量蓝量充满
    }

    // 核心函数：根据当前星级，自动刷新三维 base 值
    void calculateBaseStatsByStar()
    {
        if (starLevel >= 1)
        {
            hp.base = config->baseHp;
            atk.base = config->baseAtk;
            def.base = config->baseDef;

            // 根据星级成长系数计算属性
            for (int i = 1; i < starLevel; ++i)
            {
                hp.base = static_cast<int>(hp.base * config->hpGrowthMultiplier);
                atk.base = static_cast<int>(atk.base * config->atkGrowthMultiplier);
                // 防御力暂且不随星级成长
            }
        }
        else
        {
            // 使用图鉴的基础属性
            hp.base = config->baseHp;
            atk.base = config->baseAtk;
            def.base = config->baseDef;
        }
        attackSpeed.base = config->baseAttackSpeed;
    }

    // 回合重置：将状态恢复到备战状态
    void resetStatus()
    {
        currentHp = hp.getFinal(); // 满血
        currentMp = 0;             // 空蓝
        isAlive = true;
        targetEnemyId = -1; // 取消索敌
    }

    // 局内受伤逻辑
    void takeDamage(int rawDamage)
    {
        if (!isAlive)
            return;

        // 减伤公式：实际伤害 = 原始伤害 * (100 / (100 + 当前最终防御力))
        int finalDef = def.getFinal();
        int actualDamage = rawDamage * 100 / (100 + finalDef);
        if (actualDamage < 1)
            actualDamage = 1; // 强制保底造成 1 点伤害

        currentHp -= actualDamage;
        if (currentHp <= 0)
        {
            currentHp = 0;
            isAlive = false;
        }
    }
};

// 静态配置：角色的初始图鉴数值
struct EnemyConfig
{
    int configId; // 种类ID
    QString name; // 名字

    // 基础1星三维属性
    int baseHp;
    int baseAtk;
    int baseDef;

    // 属性成长的倍率系数
    float hpGrowthMultiplier;
    float atkGrowthMultiplier;

    int attackRange;       // 攻击距离
    float baseAttackSpeed; // 基础攻速
    double speed;          // 移动速度
    int maxMp;             // 满蓝条上限

    int baseGoldReward; // 击杀奖励金币
    int baseExpReward;  // 击杀奖励经验

    QString description; // 技能或角色描述
};

class EnemyInstance
{
public:
    // ==========================================
    // 1. 实例元数据（绑定静态图鉴）
    // ==========================================
    int uuid;                  // 自增生成uuid
    const EnemyConfig *config; // 指向对应的静态配置图鉴

    // ==========================================
    // 2. 核心战斗属性
    // ==========================================
    Attribute hp;          // 当前波次成长后的最大生命值
    Attribute atk;         // 当前波次成长后的攻击力
    Attribute def;         // 当前波次成长后的防御力
    Attribute attackSpeed; // 实时攻击速度

    // ==========================================
    // 3. 局内对战实时状态
    // ==========================================
    int currentHp = 0;   // 实时剩余血量
    bool isAlive = true; // 是否存活

    // ==========================================
    // 4. 核心机制字段
    // ==========================================
    bool isRequired;     //  true 为必修单位，false 为选修单位
    int currentTargetId; // 当前锁定的我方角色唯一ID，若为 -1 且是必修单位，则转为攻击“塔”

    // ==========================================
    // 5. 位置
    // ==========================================
    double posX = 0; // 战场网格 X
    double posY = 0; // 战场网格 Y

public:
    // 构造函数：生成敌人时，需要传入全局波次，以便让怪物属性随回合数动态成长
    EnemyInstance(int uId, const EnemyConfig *staticConfig, bool isRequired, int currentRound)
        : uuid(uId), config(staticConfig), isRequired(isRequired)
    {
        // 数值动态成长公式
        hp.base = config->baseHp;
        hp.percentBonus = (currentRound - 1) * 0.15f;

        atk.base = config->baseAtk;
        atk.percentBonus = (currentRound - 1) * 0.10f;

        def.base = config->baseDef;
        attackSpeed.base = config->baseAttackSpeed;

        // 充满状态
        currentHp = hp.getFinal();
        isAlive = true;
        currentTargetId = -1;
    }

    // 受伤逻辑（同我方一致）
    void takeDamage(int rawDamage)
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
};
// 玩家全局对局数据
struct PlayerAssets
{
    int gold;
    int exp;
    int totalCredit;                         // 累计已修学分
    int totalGradePoint;                     // 累计学分绩
    std::vector<ChessInstance> ownedChesses; // 玩家拥有的所有棋子
};

#endif
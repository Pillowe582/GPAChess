#include "state.h"
#include "renderer.h"
#include <QString>
#include "game_manager.h"

// ============================================================================
// % Transform 实现
// ============================================================================

void Transform::reset()
{
    x = y = rot = 0;
    scaleX = scaleY = 1.0;
}

// ============================================================================
// % Attribute 实现
// ============================================================================

double Attribute::getFinal() const
{
    return (base + flatBonus) * (1.0f + percentBonus);
}

// ============================================================================
// % LivingEntity 实现
// ============================================================================

LivingEntity::LivingEntity(GameManager *mgr)
    : BaseEntity(mgr)
{
    static std::atomic<int> s_counter{1};
    uuid = s_counter.fetch_add(1);
    currentHp = maxHp.getFinal();
    currentDef = maxDef.getFinal();
    currentSpeed = maxSpeed.getFinal();
}

int LivingEntity::getUuid() const
{
    return uuid;
}

double LivingEntity::getMaxHp() const
{
    return maxHp.getFinal();
}

double LivingEntity::getCurrentHp() const
{
    return currentHp;
}

double LivingEntity::getCurrentDef() const
{
    return currentDef;
}

double LivingEntity::getX() const
{
    return transform.x;
}

double LivingEntity::getY() const
{
    return transform.y;
}

void LivingEntity::setX(double v)
{
    transform.x = v;
}

void LivingEntity::setY(double v)
{
    transform.y = v;
}

void LivingEntity::setPosition(double x, double y)
{
    transform.x = x;
    transform.y = y;
}

void LivingEntity::setDeath()
{
    isAlive = false;
    currentHp = 0;
}

bool LivingEntity::stillAlive() const
{
    return isAlive;
}

double LivingEntity::dealDamage(double rawDamage)
{
    return dealDamage(rawDamage, {DamageType::Physical, QColor("#FFFFFF")});
}

double LivingEntity::dealDamage(double rawDamage, DamageType type)
{
    if (!isAlive)
        return 0.0;
    double actual = rawDamage;
    if (type.type == DamageType::Physical)
        actual *= 100.0 / (100.0 + currentDef);
    currentHp -= actual;
    if (currentHp <= 0)
        setDeath();
    gameManager->getRenderer()->queueSplash(QString("-%1").arg(static_cast<int>(actual)),
                                            transform.x, transform.y - 20,
                                            type.color);
    return actual;
}

// ============================================================================
// % ChessInstance 实现
// ============================================================================

ChessInstance::ChessInstance(const AllyConfig &cfg, GameManager *mgr)
    : AllyConfig(cfg), LivingEntity(mgr)
{
    calculateBaseStatsByStar();
    resetStatus();
}

void ChessInstance::calculateBaseStatsByStar()
{
    // 从图鉴数值初始化
    maxHp.base = baseHp;
    maxDef.base = baseDef;
    maxSpeed.base = speed;
    atk.base = baseAtk;
    for (int i = 1; i < starLevel; ++i)
    {
        maxHp.base = static_cast<int>(maxHp.base * hpGrowthMultiplier);
        atk.base = static_cast<int>(atk.base * atkGrowthMultiplier);
    }
}

void ChessInstance::resetStatus()
{
    currentHp = maxHp.getFinal();
    currentMp = 0;
    currentDef = maxDef.getFinal();
    currentSpeed = maxSpeed.getFinal();
    isAlive = true;
    targetEnemyId = -1;
}

// ============================================================================
// % EnemyInstance 实现
// ============================================================================

EnemyInstance::EnemyInstance(const EnemyConfig &cfg, bool required, int round,
                             GameManager *mgr)
    : EnemyConfig(cfg), LivingEntity(mgr), isRequired(required)
{
    // 从图鉴数值初始化
    maxHp.base = baseHp;
    maxDef.base = baseDef;
    maxSpeed.base = speed;
    atk.base = baseAtk;
    // 回合数成长
    maxHp.base = static_cast<int>(maxHp.base * (1.0 + (round - 1) * hpGrowthMultiplier));
    atk.base = static_cast<int>(atk.base * (1.0 + (round - 1) * atkGrowthMultiplier));
    currentHp = maxHp.getFinal();
    currentDef = maxDef.getFinal();
    currentSpeed = maxSpeed.getFinal();
    isAlive = true;
}

// ============================================================================
// % PlayerAssets 实现
// ============================================================================

int PlayerAssets::deployedCount() const
{
    int n = 0;
    for (auto &c : ownedChesses)
        if (c.deployed && !c.isTower)
            ++n;
    return n;
}

int PlayerAssets::benchCount() const
{
    int n = 0;
    for (auto &c : ownedChesses)
        if (!c.deployed)
            ++n;
    return n;
}

bool PlayerAssets::battlefieldFull() const
{
    return deployedCount() >= maxBattlefield;
}

bool PlayerAssets::benchFull() const
{
    return benchCount() >= maxBench;
}

void PlayerAssets::compactBenchSlots()
{
    int next = 0;
    for (auto &c : ownedChesses)
        if (!c.deployed)
            c.benchSlot = next++;
}

int PlayerAssets::firstEmptyBenchSlot() const
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

ChessInstance *PlayerAssets::getUnitByUuid(int uuid)
{
    for (auto &u : ownedChesses)
        if (u.uuid == uuid)
            return &u;
    return nullptr;
}

const ChessInstance *PlayerAssets::getUnitByUuid(int uuid) const
{
    for (auto &u : ownedChesses)
        if (u.uuid == uuid)
            return &u;
    return nullptr;
}

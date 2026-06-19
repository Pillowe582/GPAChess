#include "database_manager.h"

DatabaseManager::DatabaseManager(QObject *parent) : QObject(parent)
{
    initChessDb();
    initEnemyDb();
}

void DatabaseManager::initChessDb()
{
    // 角色A: 1000hp 50atk 100def
    {
        ChessConfig c;
        c.configId = 1;
        c.name = QStringLiteral("角色A");
        c.cost = 1;
        c.baseHp = 1000;
        c.baseAtk = 50;
        c.baseDef = 100;
        c.hpGrowthMultiplier = 1.50f;
        c.atkGrowthMultiplier = 1.50f;
        c.attackRange = 1;
        c.baseAttackSpeed = 1.0f;
        c.maxMp = 100;
        c.speed = 1.0;
        c.description = QStringLiteral("高血量坦克型角色");
        c.bonds = {QStringLiteral("坦克")};
        m_chessConfigs.push_back(c);
    }
    // 角色B: 500hp 100atk 200def
    {
        ChessConfig c;
        c.configId = 2;
        c.name = QStringLiteral("角色B");
        c.cost = 1;
        c.baseHp = 500;
        c.baseAtk = 100;
        c.baseDef = 200;
        c.hpGrowthMultiplier = 1.50f;
        c.atkGrowthMultiplier = 1.50f;
        c.attackRange = 1;
        c.baseAttackSpeed = 1.0f;
        c.maxMp = 100;
        c.speed = 1.0;
        c.description = QStringLiteral("高防御输出型角色");
        c.bonds = {QStringLiteral("输出")};
        m_chessConfigs.push_back(c);
    }
}

void DatabaseManager::initEnemyDb()
{
    // 敌人X: 1000hp 50atk 100def
    {
        EnemyConfig c;
        c.configId = 1;
        c.name = QStringLiteral("敌人X");
        c.baseHp = 500;
        c.baseAtk = 50;
        c.baseDef = 100;
        c.hpGrowthMultiplier = 1.30f;
        c.atkGrowthMultiplier = 1.30f;
        c.baseAttackSpeed = 1.0f;
        c.speed = 1.0;
        c.maxMp = 0;
        c.baseGoldReward = 3;
        c.baseExpReward = 2;
        c.description = QStringLiteral("敌方坦克");
        m_enemyConfigs.push_back(c);
    }
    // 敌人Y: 500hp 100atk 200def
    {
        EnemyConfig c;
        c.configId = 2;
        c.name = QStringLiteral("敌人Y");
        c.baseHp = 250;
        c.baseAtk = 100;
        c.baseDef = 200;
        c.hpGrowthMultiplier = 1.30f;
        c.atkGrowthMultiplier = 1.30f;
        c.baseAttackSpeed = 1.0f;
        c.speed = 1.0;
        c.maxMp = 0;
        c.baseGoldReward = 3;
        c.baseExpReward = 2;
        c.description = QStringLiteral("敌方输出");
        m_enemyConfigs.push_back(c);
    }
}

const ChessConfig *DatabaseManager::findChess(int configId) const
{
    for (const auto &c : m_chessConfigs)
        if (c.configId == configId)
            return &c;
    return nullptr;
}

const EnemyConfig *DatabaseManager::findEnemy(int configId) const
{
    for (const auto &c : m_enemyConfigs)
        if (c.configId == configId)
            return &c;
    return nullptr;
}

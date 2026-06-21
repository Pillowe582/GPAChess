#include "database_manager.h"
#include <QCoreApplication>
#include <QSqlQuery>
#include <QSqlError>

DatabaseManager::DatabaseManager(const QString &appDir, QObject *parent)
    : QObject(parent)
{
    openDb(appDir + "/assets/database/role.db", "role_conn", m_chessDb);
    openDb(appDir + "/assets/database/enemy.db", "enemy_conn", m_enemyDb);
    loadFromDb();
}

/// @brief 打开数据库连接
/// @param path 数据库文件路径
/// @param connName 该数据库连接的名称
/// @param outDb 输出参数：打开的数据库连接
void DatabaseManager::openDb(const QString &path, const QString &connName, QSqlDatabase &outDb)
{
    outDb = QSqlDatabase::addDatabase("QSQLITE", connName);
    outDb.setDatabaseName(path);
    if (!outDb.open())
        qFatal("Cannot open database: %s", qPrintable(path));
}

/// @brief 从数据库加载配置数据到内存
void DatabaseManager::loadFromDb()
{
    m_allyConfigs.clear();
    m_enemyConfigs.clear();

    // 验证并加载 chess_config
    bool hasChessData = false;
    QSqlQuery chessSqlQuery(m_chessDb);
    chessSqlQuery.exec("SELECT * FROM chess_config");
    while (chessSqlQuery.next())
    {
        hasChessData = true; // 只要能读到至少一行数据就认为表存在且不空
        AllyConfig allyConfigItem;
        allyConfigItem.configId = chessSqlQuery.value(0).toInt();
        allyConfigItem.name = chessSqlQuery.value(1).toString();
        allyConfigItem.cost = chessSqlQuery.value(2).toInt();
        allyConfigItem.baseHp = chessSqlQuery.value(3).toInt();
        allyConfigItem.baseAtk = chessSqlQuery.value(4).toInt();
        allyConfigItem.baseDef = chessSqlQuery.value(5).toInt();
        allyConfigItem.hpGrowthMultiplier = chessSqlQuery.value(6).toFloat();
        allyConfigItem.atkGrowthMultiplier = chessSqlQuery.value(7).toFloat();
        allyConfigItem.attackRange = chessSqlQuery.value(8).toInt();
        allyConfigItem.baseAttackSpeed = chessSqlQuery.value(9).toFloat();
        allyConfigItem.maxMp = chessSqlQuery.value(10).toInt();
        allyConfigItem.speed = chessSqlQuery.value(11).toDouble();
        allyConfigItem.description = chessSqlQuery.value(12).toString();
        allyConfigItem.bonds = chessSqlQuery.value(13).toString().split(',', Qt::SkipEmptyParts);
        allyConfigItem.behaviorId = chessSqlQuery.value(14).toInt();
        m_allyConfigs.push_back(allyConfigItem);
    }
    if (!hasChessData)
    {
        qFatal("role.db: table 'chess_config' is missing or empty");
    }

    // 验证并加载 enemy_config
    bool hasEnemyData = false;
    QSqlQuery enemySqlQuery(m_enemyDb);
    enemySqlQuery.exec("SELECT * FROM enemy_config");
    while (enemySqlQuery.next())
    {
        hasEnemyData = true; // 只要能读到至少一行数据就认为表存在且不空
        EnemyConfig enemyConfigItem;
        enemyConfigItem.configId = enemySqlQuery.value(0).toInt();
        enemyConfigItem.name = enemySqlQuery.value(1).toString();
        enemyConfigItem.baseHp = enemySqlQuery.value(2).toInt();
        enemyConfigItem.baseAtk = enemySqlQuery.value(3).toInt();
        enemyConfigItem.baseDef = enemySqlQuery.value(4).toInt();
        enemyConfigItem.hpGrowthMultiplier = enemySqlQuery.value(5).toFloat();
        enemyConfigItem.atkGrowthMultiplier = enemySqlQuery.value(6).toFloat();
        enemyConfigItem.attackRange = enemySqlQuery.value(7).toInt();
        enemyConfigItem.baseAttackSpeed = enemySqlQuery.value(8).toFloat();
        enemyConfigItem.maxMp = enemySqlQuery.value(9).toInt();
        enemyConfigItem.speed = enemySqlQuery.value(10).toDouble();
        enemyConfigItem.description = enemySqlQuery.value(11).toString();
        enemyConfigItem.baseGoldReward = enemySqlQuery.value(12).toInt();
        enemyConfigItem.baseExpReward = enemySqlQuery.value(13).toInt();
        enemyConfigItem.behaviorId = enemySqlQuery.value(14).toInt();
        m_enemyConfigs.push_back(enemyConfigItem);
    }
    if (!hasEnemyData)
    {
        qFatal("enemy.db: table 'enemy_config' is missing or empty");
    }
}

/// @brief  根据 configId 查找棋子配置
/// @param configId
/// @return 找到则返回指向配置的指针，否则返回 nullptr
const AllyConfig *DatabaseManager::findAlly(int configId) const
{
    for (const auto &c : m_allyConfigs)
    {
        if (c.configId == configId)

            return &c;
    }
    return nullptr;
}

/// @brief  根据 configId 查找敌人配置
/// @param configId
/// @return  找到则返回指向配置的指针，否则返回 nullptr
const EnemyConfig *DatabaseManager::findEnemy(int configId) const
{
    for (const auto &c : m_enemyConfigs)
    {
        if (c.configId == configId)
            return &c;
    }
    return nullptr;
}

#include "database_manager.h"
#include <QCoreApplication>
#include <QSqlQuery>
#include <QSqlError>

DatabaseManager::DatabaseManager(const QString &dataDir, QObject *parent)
    : QObject(parent)
{
    openDb(dataDir + "/role.db", "role_conn", m_chessDb);
    openDb(dataDir + "/enemy.db", "enemy_conn", m_enemyDb);
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
    m_chessConfigs.clear();
    m_enemyConfigs.clear();

    // 验证并加载 chess_config
    bool hasChessData = false;
    QSqlQuery chessSqlQuery(m_chessDb);
    chessSqlQuery.exec("SELECT * FROM chess_config");
    while (chessSqlQuery.next())
    {
        hasChessData = true; // 只要能读到至少一行数据就认为表存在且不空
        ChessConfig chessConfigItem;
        chessConfigItem.configId = chessSqlQuery.value(0).toInt();
        chessConfigItem.name = chessSqlQuery.value(1).toString();
        chessConfigItem.cost = chessSqlQuery.value(2).toInt();
        chessConfigItem.baseHp = chessSqlQuery.value(3).toInt();
        chessConfigItem.baseAtk = chessSqlQuery.value(4).toInt();
        chessConfigItem.baseDef = chessSqlQuery.value(5).toInt();
        chessConfigItem.hpGrowthMultiplier = chessSqlQuery.value(6).toFloat();
        chessConfigItem.atkGrowthMultiplier = chessSqlQuery.value(7).toFloat();
        chessConfigItem.attackRange = chessSqlQuery.value(8).toInt();
        chessConfigItem.baseAttackSpeed = chessSqlQuery.value(9).toFloat();
        chessConfigItem.maxMp = chessSqlQuery.value(10).toInt();
        chessConfigItem.speed = chessSqlQuery.value(11).toDouble();
        chessConfigItem.description = chessSqlQuery.value(12).toString();
        chessConfigItem.bonds = chessSqlQuery.value(13).toString().split(',', Qt::SkipEmptyParts);
        m_chessConfigs.push_back(chessConfigItem);
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
const ChessConfig *DatabaseManager::findChess(int configId) const
{
    for (const auto &c : m_chessConfigs)
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

#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QString>
#include <vector>
#include "state.h"

class DatabaseManager : public QObject
{
    Q_OBJECT
public:
    explicit DatabaseManager(const QString &dataDir, QObject *parent = nullptr);

    const std::vector<ChessConfig> &allChessConfigs() const { return m_chessConfigs; }
    const std::vector<EnemyConfig> &allEnemyConfigs() const { return m_enemyConfigs; }
    const ChessConfig *findChess(int configId) const;
    const EnemyConfig *findEnemy(int configId) const;

private:
    void openDb(const QString &path, const QString &connName, QSqlDatabase &outDb);
    void loadFromDb();

    QSqlDatabase m_chessDb;
    QSqlDatabase m_enemyDb;
    std::vector<ChessConfig> m_chessConfigs;
    std::vector<EnemyConfig> m_enemyConfigs;
};

#endif

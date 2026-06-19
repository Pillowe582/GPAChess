#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <vector>
#include "state.h"

class DatabaseManager : public QObject
{
    Q_OBJECT
public:
    explicit DatabaseManager(QObject *parent = nullptr);

    const std::vector<ChessConfig> &allChessConfigs() const { return m_chessConfigs; }
    const std::vector<EnemyConfig> &allEnemyConfigs() const { return m_enemyConfigs; }
    const ChessConfig *findChess(int configId) const;
    const EnemyConfig *findEnemy(int configId) const;

private:
    void initChessDb();
    void initEnemyDb();
    std::vector<ChessConfig> m_chessConfigs;
    std::vector<EnemyConfig> m_enemyConfigs;
};

#endif

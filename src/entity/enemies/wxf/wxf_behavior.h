#ifndef WXF_ENEMY_H
#define WXF_ENEMY_H
#include "entity/enemy_behavior.h"
#include <memory>
#include <QVector2D>

class ChessInstance;

/// WXF 敌方：近战，找最近我方单位，移动到70px后3圈武器挥砍
class WXFEnemy : public EnemyBehavior
{
public:
    void tick(double dt, EnemyInstance &self,
              std::vector<std::unique_ptr<ChessInstance>> &allies,
              Renderer &renderer,
              int &pendingGold, int &pendingExp) override;

private:
    struct Weapon
    {
        bool active = false;
        double angle = 0.0;
        double elapsed = 0.0;
        double rotSpeed = -0.5f; // 初始旋转速度
        int rotationsDone = 0;
        int targetUuid = -1;
    };
    QVector2D m_targetVec; // 当前攻击目标向量
    Weapon m_weapon;
    double m_cooldown = 0.0;
};

#endif

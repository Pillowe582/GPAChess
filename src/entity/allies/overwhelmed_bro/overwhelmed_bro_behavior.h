#ifndef OVERWHHELMED_BRO_ALLY_H
#define OVERWHHELMED_BRO_ALLY_H
#include "entity/ally_behavior.h"
#include <QVector2D>

class GameManager;

/// OverwhelmedBro 我方：远程，找最远敌人，发射子弹，不移动
class OverwhelmedBroAlly : public AllyBehavior
{
public:
    void tick(double dt, BaseEntity &self, GameManager &gameManager) override;

    void onStart(AllyInstance &self);

private:
    // ---- 此角色自有的子弹数据 ----
    struct Bullet
    {
        double x = 0, y = 0;
        double vx = 0, vy = 0;
        double rot = 0;
        int damage = 0;
    };
    struct TropicalTeaSpout
    {
        double x = 0, y = 0;
        double rot = 0;
        double timePassed = -1;
        int hitCount = 0;
        int damage = 0;
    };
    std::vector<Bullet> m_bullets;

    TropicalTeaSpout m_teaSpout;

    double m_maxCooldown = 1.5; // 发射冷却
    double m_currentCooldown = 0.0;

    QVector2D m_targetDirection; // 指向目标的向量，用于子弹发射方向

    void updatePos(double dt, AllyInstance &self, const std::vector<EnemyInstance *> &enemies);

    // % 索敌方法

    /// @brief 索敌
    /// @note 使用曼哈顿距离
    /// @param enemies 所有敌人向量
    /// @param self 自身实例
    /// @param bestDist 最佳距离，最远时传入-1，最远时传入114514
    /// @return 返回找到的敌人指针，若无则返回 nullptr
    EnemyInstance *findEnemy(const std::vector<EnemyInstance *> &enemies,
                             const AllyInstance &self, int bestDist = -1);

    // % 发射泸溪河技能

    /// @brief 子弹推进、渲染、碰撞检测
    /// @param dt 经过的时间
    /// @param self 自身实例
    /// @param gameManager 游戏管理器
    void updateBullets(double dt, AllyInstance &self, GameManager &gameManager);

    /// 向目标发射一颗子弹，存入 m_bullets
    void fireBullet(const AllyInstance &self, EnemyInstance *target);

    // % 热带风味喷射战士技能
    void updateTropicalTeaSpout(double dt, AllyInstance &self, GameManager &gameManager);
};

#endif
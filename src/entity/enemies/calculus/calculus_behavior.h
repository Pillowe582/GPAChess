#ifndef CALCULUS_ENEMY_H
#define CALCULUS_ENEMY_H
#include "entity/enemy_behavior.h"
#include <memory>
#include <QVector2D>

class AllyInstance;
class GameManager;

/// 微积分 敌方：近战，找最近我方单位，移动到70px后，挥舞武器
class CalculusEnemy : public EnemyBehavior
{
public:
    void tick(double dt, BaseEntity &self, GameManager &gameManager) override;

    void onStart(EnemyInstance &self);

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

    /// 处理武器挥砍动画、碰撞检测和渲染
    void updateInteger(double dt,
                       const std::vector<std::unique_ptr<AllyInstance>> &allies,
                       Renderer &renderer,
                       EnemyInstance &self);

    /// 找最近的我方单位（优先排除塔），返回 nullptr 表示无目标
    AllyInstance *findClosestAlly(const std::vector<std::unique_ptr<AllyInstance>> &allies,
                                  const EnemyInstance &self);

    /// @brief 寻找最近的上下界目标（infX < x < supX）
    /// @param allies 我方单位列表
    /// @param self 敌人自身
    /// @param infX 最靠左的单位的x回调
    /// @param supX 最靠右的单位的x回调
    void findInfSupAlly(const std::vector<std::unique_ptr<AllyInstance>> &allies,
                        const EnemyInstance &self,
                        double &infX, double &supX);

    /// 向目标移动（远距离靠近，近距离后退）
    void updatePos(double dt, EnemyInstance &self, AllyInstance *target);
};

#endif
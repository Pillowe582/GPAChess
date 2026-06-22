#ifndef CALCULUS_ENEMY_H
#define CALCULUS_ENEMY_H
#include "entity/enemy_behavior.h"
#include <memory>
#include <QPen>
#include <vector>
#include <utility>
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
    // %数据定义
    struct IntegerWeapon
    {
        bool active = false;
        double angle = 0.0;
        double elapsed = 0.0;
        double rotSpeed = -0.5f; // 初始旋转速度
        int rotationsDone = 0;
        int targetUuid = -1;
        void reset()
        {
            active = false;
            angle = 0.0;
            elapsed = 0.0;
            rotSpeed = -0.5f;
            rotationsDone = 0;
            targetUuid = -1;
        }
    };
    struct Differential
    {
        struct TempLine
        {
            double x;
            double bornTime;
        };
        bool active = false;
        double elapsedTime = 0.0;
        int roundCount = 0;
        double groupElapsed = 0.0;     // 组计时器
        double lineSpawnTimer = 0.0;   // 线生成计时器
        double sectionWidth = 0.0;     // 每段宽度（根据当前轮数计算）
        int currentGroupLineCount = 0; // 当前组线数量
        int linesSpawnedInGroup = 0;   // 当前组已生成数量
        double infX = 0.0;
        double supX = 0.0;
        double damage = 0.0;
        std::vector<TempLine> activeLines;
        void reset()
        {
            active = false;
            elapsedTime = 0.0;
            roundCount = 0;
            groupElapsed = 0.0;
            lineSpawnTimer = 0.0;
            currentGroupLineCount = 0;
            linesSpawnedInGroup = 0;
            infX = 0.0;
            supX = 0.0;
            damage = 0.0;
            activeLines.clear();
        }
    };
    QVector2D m_targetVec; // 当前攻击目标向量
    IntegerWeapon m_weapon;

    QPen m_differentialPen;
    Differential m_differential;

    double m_cooldown = 0.0;

    /// 向目标移动（远距离靠近，近距离后退）
    void updatePos(double dt, EnemyInstance &self, AllyInstance *target);

    // % 工具方法
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

    // % 普攻
    /// 处理武器挥砍动画、碰撞检测和渲染
    void updateInteger(double dt,
                       const std::vector<std::unique_ptr<AllyInstance>> &allies,
                       Renderer &renderer,
                       EnemyInstance &self);

    // % 终结技

    void spawnLine(AllyInstance &ally, double lineX, double dmg, EnemyInstance &self);

    /// 更新大招状态
    void updateDifferential(double dt,
                            const std::vector<std::unique_ptr<AllyInstance>> &allies,
                            Renderer &renderer,
                            EnemyInstance &self);
};

#endif
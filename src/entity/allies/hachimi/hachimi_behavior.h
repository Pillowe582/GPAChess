#ifndef HACHIMI_ALLY_H
#define HACHIMI_ALLY_H
#include "entity/ally_behavior.h"

class QString;
class QRandomGenerator;
class GameManager;

/// Hachimi 我方：近战，找最近敌人，移动到70px距离后3圈武器挥砍
class HachimiAlly : public AllyBehavior
{
public:
    void tick(double dt, BaseEntity &self, GameManager &gameManager) override;

    void onStart(AllyInstance &self);

    QString getTexturePath(const AllyInstance &self) const override;

    // % 哈基米状态定义
    struct HachimiState
    {
        enum State
        {
            DoWhat,
            Ha,
            O
        };
        State state = DoWhat;
        int attackedRounds = 0;      // 记录攻击了多少次敌人
        double m_oStateTimer = 0.0;  // O状态计时器
        double m_haStateTimer = 0.0; // 哈气状态计时器
        void reset()
        {
            state = DoWhat;
            attackedRounds = 0;
            m_oStateTimer = 0.0;
            m_haStateTimer = 0.0;
        };
    };

private:
    double m_lastRecordedHp = 0.0;        // 上次记录的HP，用于检测是否被攻击
    double m_lastNotHurtTimer = 0.0;      // 记录上次被攻击的时间，用于触发MP增加
    GameManager *m_gameManager = nullptr; // 游戏管理器引用

    HachimiState m_state; // 角色状态
    double m_cooldown = 0.0;

    EnemyInstance *m_target = nullptr;

    void switchState(HachimiState::State newState);

    // 索敌
    void findNearestTarget(const std::vector<EnemyInstance *> &enemies,
                           const AllyInstance &self);

    void findRandomTarget(const std::vector<EnemyInstance *> &enemies,
                          const AllyInstance &self, QRandomGenerator *rng);

    // 走位
    void updatePos(double dt, AllyInstance &self,
                   const std::vector<EnemyInstance *> &enemies,
                   GameManager &gameManager);

    // 攻击
    void performHaAttack(AllyInstance &self, GameManager &gameManager);

    // 亡语
    void onDeath(AllyInstance *self) override;
};

#endif
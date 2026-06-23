#ifndef PHYSICS_ENEMY_H
#define PHYSICS_ENEMY_H
#include "entity/enemy_behavior.h"
#include <memory>

class AllyInstance;
class GameManager;
class QRandomGenerator;
class GameEntities;

/// Physics 敌方：远程，找最远我方单位，发射子弹；无目标时攻击塔
class PhysicsEnemy : public EnemyBehavior
{
public:
    void tick(double dt, BaseEntity &self, GameManager &gameManager) override;

    void onStart(EnemyInstance &self) override;

private:
    // % 定义场的结构体
    struct ElectricField
    {
        bool active = false;
        int waveIndex = 0;      // 当前第几波 (0~2)
        double waveTimer = 0.0; // 当前波纹已经过的时间
        double gapTimer = 0.0;  // 波纹之间间隔计时
        double centerX = 0.0, centerY = 0.0;
        bool waveDamageDealt = false; // 当前波纹是否已造成伤害
        void reset()
        {
            active = false;
            waveIndex = 0;
            waveTimer = 0.0;
            gapTimer = 0.0;
            centerX = 0.0;
            centerY = 0.0;
            waveDamageDealt = false;
        }
    } m_electricField;

    struct CurlField
    {
        bool active = false;
        double timer = 0.0;
        static constexpr double duration = 2;   // 总持续 1s
        static constexpr double radius = 500.0; // 半径 700px
        bool firstHalfDamageDealt = false;      // 前半段伤害
        bool secondHalfHealDealt = false;       // 后半段治疗

        void reset()
        {
            active = false;
            timer = 0.0;
            firstHalfDamageDealt = false;
            secondHalfHealDealt = false;
        }
    } m_curlField;

    double m_lastRecordedHp = 0.0;
    double m_blockingEffectTimer = 0.0; // 格挡效果持续时间

    double m_cooldown = 0.0;
    // 攻击调度
    void updateAttack(double dt, EnemyInstance &self,
                      std::vector<std::unique_ptr<AllyInstance>> &allies,
                      std::vector<EnemyInstance *> &enemies, Renderer &renderer);

    // 一技能
    void summonElectricField(EnemyInstance *target);
    void updateElectricField(double dt, std::vector<std::unique_ptr<AllyInstance>> &allies,
                             Renderer &renderer, EnemyInstance &self);
    void dealElectricDamage(std::vector<std::unique_ptr<AllyInstance>> &allies, EnemyInstance &self,
                            double centerX, double centerY, double radius);
    void pullAlliesTowardCenter(std::vector<std::unique_ptr<AllyInstance>> &allies,
                                double centerX, double centerY, double radius, double dt);
    void renderElectricWave(Renderer &renderer, double centerX, double centerY,
                            double radius, double waveTimer);
    EnemyInstance *findRandomEnemy(const std::vector<EnemyInstance *> &enemies, QRandomGenerator *rng);

    // 二技能
    void checkHp(EnemyInstance &self, Renderer &renderer);
    void updateBlockingEffect(double dt, EnemyInstance &self, Renderer &renderer);

    // 三技能

    void summonCurlField(EnemyInstance &self);
    void updateCurlField(double dt, std::vector<std::unique_ptr<AllyInstance>> &allies,
                         std::vector<EnemyInstance *> &enemies,
                         Renderer &renderer, EnemyInstance &self);

    void applyCurlRotation(std::vector<std::unique_ptr<AllyInstance>> &allies,
                           std::vector<EnemyInstance *> &enemies, EnemyInstance &self,
                           double dt, bool clockwise);
    void dealCurlDamage(std::vector<std::unique_ptr<AllyInstance>> &allies, EnemyInstance &self);
    void dealCurlHeal(std::vector<EnemyInstance *> &enemies, EnemyInstance &self, Renderer &renderer);
    void renderCurlEffect(Renderer &renderer, EnemyInstance &self, bool clockwise);

    // 走位
    void updatePos(double dt, EnemyInstance &self,
                   std::vector<std::unique_ptr<AllyInstance>> &allies);
};

#endif

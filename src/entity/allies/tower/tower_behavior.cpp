#include "allies/tower/tower_behavior.h"
#include "state.h"
#include "renderer.h"
#include "game_manager.h"

#include <algorithm>
#include <QRandomGenerator>
#include <QPainter>
#include <QPen>
#include <QFont>

// % 回合开始
void TowerBehavior::onStart(AllyInstance &self)
{
    // 战斗开始时设置初始攻击冷却，避免塔立即开火
    m_currentAttackCooldown = m_maxAttackCooldown; // 塔的攻击间隔为1.5秒
}

//% 每逻辑Tick
void TowerBehavior::tick(double dt, BaseEntity &baseSelf, GameManager &gameManager)
{
    AllyInstance &self = static_cast<AllyInstance &>(baseSelf);

    m_hpRatio = self.maxHp.getFinal() > 0
                    ? self.currentHp / self.maxHp.getFinal()
                    : 1.0;
    attack(dt, self, gameManager); // 执行攻击逻辑
}

// % 攻击逻辑
void TowerBehavior::attack(double dt, AllyInstance &self, GameManager &gameManager)
{
    auto &enemies = gameManager.getGameEntities().enemies;
    auto rng = QRandomGenerator::global();

    m_currentAttackCooldown = std::max(0.0, m_currentAttackCooldown - dt);
    if (m_currentAttackCooldown > 0.0)
        return;

    // 找第一个存活敌人
    auto it = std::find_if(enemies.begin(), enemies.end(),
                           [](const EnemyInstance *e)
                           { return e->isAlive && e->isRequired; });
    if (it == enemies.end())
        return;

    // 伤害计算
    EnemyInstance *target = *it;
    int dmg = static_cast<int>(target->maxHp.getFinal() * (0.01 + (1.0 - m_hpRatio) * 2.0));

    target->dealDamage(dmg, self, DamageType{DamageType::True, QColor("#46a6ff")});

    if (!target->isAlive)
    {
        int gold = (rng->bounded(10) == 0) ? target->baseGoldReward : 0;
        gameManager.addPendingRewards(gold, target->baseExpReward);
    }

    m_currentAttackCooldown = m_maxAttackCooldown;
}

// % Queue 渲染
void TowerBehavior::renderSelf(const AllyInstance &self, Renderer &renderer,
                               double x, double y)
{
    const double tx = 10.0, ts = 128.0, ty = 400.0 - ts / 2.0;
    QString texturePath = ":/texture/entity/tower.png";
    renderer.queueImage(texturePath, tx + ts / 2.0, ty + ts / 2.0, 0.0, 1.0, Qt::AlignCenter, 10);

    // 根据血量比例选择颜色
    QColor hpC = m_hpRatio > 0.5    ? QColor(80, 180, 255)
                 : m_hpRatio > 0.25 ? QColor(255, 200, 60)
                                    : QColor(255, 70, 70);
    renderer.queueRect(tx, ty - 12.0, ts * m_hpRatio, 6.0, hpC, 7);

    // 绘制 GPA 数字
    double gpa = m_hpRatio * 4.0;
    renderer.queueText(QString::number(gpa, 'f', 1),
                       tx + ts / 2.0 - 30.0, ty + ts / 2.0 - 30.0,
                       TextStyle().setFontSize(20).setBold(true).setColor(QColor("#46a6ff")), 20);
}
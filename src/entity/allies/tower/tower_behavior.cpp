#include "allies/tower/tower_behavior.h"
#include "state.h"
#include "renderer.h"
#include "game_manager.h"

#include <algorithm>
#include <QRandomGenerator>
#include <QPainter>
#include <QPen>
#include <QEasingCurve>
#include <QFont>

// % 回合开始
void TowerBehavior::onStart(AllyInstance &self)
{
    // 战斗开始时设置初始攻击冷却，避免塔立即开火
    m_laserPen = QPen(QColor("#d93379"), 3.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
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
    Renderer &renderer = gameManager.getRenderer();

    // 更新激光动画计时器
    m_laserTimer = std::max(0.0, m_laserTimer - dt);

    // 冷却中
    m_currentAttackCooldown = std::max(0.0, m_currentAttackCooldown - dt);
    if (m_currentAttackCooldown > 0.0)
    {
        // 冷却期间继续渲染激光动画（如果有的话）
        if (m_laserTarget && m_laserTimer > 0.0)
        {
            renderLaserAnimation(renderer, self, *m_laserTarget);
        }
        return;
    }

    // 找第一个存活敌人（优先必修）
    auto it = std::find_if(enemies.begin(), enemies.end(),
                           [](const EnemyInstance *e)
                           { return e->isAlive && e->isRequired; });
    if (it == enemies.end())
        return;

    // 伤害计算
    EnemyInstance *target = *it;
    int dmg = static_cast<int>(target->maxHp.getFinal() * (0.01 + (1.0 - m_hpRatio) * 2.0));

    target->dealDamage(dmg, self, DamageType{DamageType::True, QColor("#d93379"), QColor("#660874")});

    // 设置激光动画
    m_laserTarget = target;
    m_laserTimer = m_maxAttackCooldown; // 激光持续时间 = 攻击冷却时间

    if (!target->isAlive)
    {
        int gold = (rng->bounded(10) == 0) ? target->baseGoldReward : 0;
        gameManager.addPendingRewards(gold, target->baseExpReward);
    }

    m_currentAttackCooldown = m_maxAttackCooldown;
}

// % 激光渲染

// 渲染激光动画
void TowerBehavior::renderLaserAnimation(Renderer &renderer, AllyInstance &self, EnemyInstance &target)
{
    const double laserDuration = 0.3; // 激光总持续时间 0.3s
    const double emitDuration = 0.15; // 发射阶段 0.15s
    const double fadeDuration = 0.15; // 消失阶段 0.15s

    // 当前激光已过去的时间
    double elapsedTime = m_maxAttackCooldown - m_currentAttackCooldown;
    // 或者用 m_laserTimer 的反向值：
    // double laserElapsed = laserDuration - m_laserTimer;

    double progress = 0.0;
    double alpha = 1.0;

    if (elapsedTime <= emitDuration)
    {
        // === 发射阶段 (0 ~ 0.15s)：激光从塔发出连接到敌人 ===
        progress = elapsedTime / emitDuration; // 0 → 1

        // 使用缓动函数让发射更自然
        QEasingCurve curve(QEasingCurve::OutCubic);
        progress = curve.valueForProgress(progress);

        alpha = progress; // 透明度从 0 → 1
    }
    else if (elapsedTime <= laserDuration)
    {
        // === 消失阶段 (0.15s ~ 0.3s)：激光从塔开始消失 ===
        double fadeTime = elapsedTime - emitDuration;
        progress = 1.0 - (fadeTime / fadeDuration); // 1 → 0

        // 缓动消失
        QEasingCurve curve(QEasingCurve::InCubic);
        progress = curve.valueForProgress(progress);

        alpha = progress; // 透明度从 1 → 0
    }
    else
    {
        // 激光结束
        return;
    }

    double startX = self.getX();
    double startY = self.getY();
    double endX = target.getX();
    double endY = target.getY();

    // 激光发射方向单位向量
    double dx = endX - startX;
    double dy = endY - startY;
    double dist = std::sqrt(dx * dx + dy * dy);
    if (dist < 0.001)
        return;

    double ux = dx / dist;
    double uy = dy / dist;

    // 发射阶段：激光从塔出发，逐渐延伸到敌人
    // 消失阶段：激光从塔开始消失，终点向塔收缩
    double currentEndX, currentEndY;

    if (elapsedTime <= emitDuration)
    {
        // 发射阶段：终点从塔延伸到敌人
        currentEndX = startX + ux * dist * progress;
        currentEndY = startY + uy * dist * progress;
    }
    else
    {
        // 消失阶段：起点从塔向敌人移动（激光尾部消失）
        double fadeStart = dist * (1.0 - progress);
        currentEndX = startX + ux * dist; // 终点始终在敌人
        currentEndY = startY + uy * dist;

        // 调整起点
        startX = startX + ux * fadeStart;
        startY = startY + uy * fadeStart;
    }

    // 绘制激光主线
    renderer.queueLine(startX, startY, currentEndX, currentEndY, m_laserPen, 100);

    renderer.queueLine(startX + 2, startY + 2, currentEndX + 2, currentEndY + 2, m_laserPen.color().lighter(150), 99);

    // 火花花
    if (elapsedTime <= emitDuration)
    {
        double sparkAlpha = progress * alpha;
        renderer.queueSplash("*", currentEndX, currentEndY,
                             QColor(m_laserPen.color().red(), m_laserPen.color().green(), m_laserPen.color().blue(),
                                    static_cast<int>(16 + sparkAlpha * 239)),
                             QColor(Qt::transparent),
                             100);
    }
}

// % Queue 渲染
void TowerBehavior::renderSelf(const AllyInstance &self, Renderer &renderer,
                               double x, double y)
{
    QString texturePath = ":/texture/entity/tower.png";
    renderer.queueImage(texturePath, x, y, 0.0, 1.0, Qt::AlignCenter, 10);

    // 根据血量比例选择颜色
    QColor hpC = m_hpRatio > 0.667  ? QColor("#00ff00")
                 : m_hpRatio > 0.25 ? QColor("#ffff00")
                                    : QColor("#ff0000");
    renderer.queueRect(x - 64.0, y - 64.0, 128.0 * m_hpRatio, 6.0, hpC, 7);

    // 绘制 GPA 数字
    double gpa = m_hpRatio * 4.0;
    renderer.queueText(QString::number(gpa, 'f', 1),
                       x - 40, y - 40,
                       TextStyle().setFontSize(20).setBold(true).setColor(hpC), 200);

    renderer.queueText(QString::number(gpa, 'f', 1),
                       x - 38, y - 38,
                       TextStyle().setFontSize(20).setBold(true).setColor(Qt::black), 199);
}
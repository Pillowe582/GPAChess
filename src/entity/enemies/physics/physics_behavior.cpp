#include "physics_behavior.h"
#include "state.h"
#include "renderer.h"
#include "game_manager.h"
#include <cmath>
#include <QRandomGenerator>
#include <QEasingCurve>

// % 初始化
void PhysicsEnemy::onStart(EnemyInstance &self)
{
    m_electricField.reset();
    m_lastRecordedHp = self.currentHp;
    m_cooldown = 2; // 初始处于冷却
}

// % 每刻逻辑
void PhysicsEnemy::tick(double dt, BaseEntity &baseSelf, GameManager &gameManager)
{
    EnemyInstance &self = static_cast<EnemyInstance &>(baseSelf);
    auto &allies = gameManager.getPlayerAssets().ownedChesses;
    auto &enemies = gameManager.getGameEntities().enemies;
    Renderer &renderer = gameManager.getRenderer();

    updatePos(dt, self, allies); // 走位
    checkHp(self, renderer);

    // 刷新特效
    updateBlockingEffect(dt, self, renderer);
    updateElectricField(dt, allies, renderer, self);

    // 攻击调度
    updateAttack(dt, self, allies, enemies, renderer);

    m_lastRecordedHp = self.currentHp;
}

// % 攻击调度
void PhysicsEnemy::updateAttack(double dt, EnemyInstance &self,
                                std::vector<std::unique_ptr<AllyInstance>> &allies,
                                std::vector<EnemyInstance *> &enemies,
                                Renderer &renderer)
{
    //  旋度场进行中,优先级最高
    if (m_curlField.active)
    {
        updateCurlField(dt, allies, enemies, renderer, self);
        return;
    }
    // 尝试触发旋度场
    summonCurlField(self);
    if (m_curlField.active)
        return;

    // 电场进行中则不发动新攻击
    if (m_electricField.active)
        return;

    m_cooldown = std::max(0.0, m_cooldown - dt);
    if (m_cooldown > 0.0)
        return;

    EnemyInstance *target = findRandomEnemy(enemies, QRandomGenerator::global());
    if (target)
        summonElectricField(target);
}

// % 随机寻找一个己方目标
EnemyInstance *PhysicsEnemy::findRandomEnemy(const std::vector<EnemyInstance *> &enemies, QRandomGenerator *rng)
{
    std::vector<EnemyInstance *> alivePreferentialEnemies;
    std::vector<EnemyInstance *> aliveEnemies;
    for (auto *e : enemies)
    {
        if (e->isAlive)
        {
            aliveEnemies.push_back(e);
            if (e->behaviorId == 1) // 额外把Calculus加入技能优先目标
                alivePreferentialEnemies.push_back(e);
        }
    }
    if (aliveEnemies.empty())
        return nullptr;

    if (!alivePreferentialEnemies.empty())
    {
        int idx = rng->bounded(static_cast<int>(alivePreferentialEnemies.size()));
        return alivePreferentialEnemies[idx];
    }
    int idx = rng->bounded(static_cast<int>(aliveEnemies.size()));
    return aliveEnemies[idx];
}
// % 电场

// 生成
void PhysicsEnemy::summonElectricField(EnemyInstance *target)
{
    m_electricField.reset();
    m_electricField.active = true;
    m_electricField.centerX = target->getX();
    m_electricField.centerY = target->getY();
}

// 更新
void PhysicsEnemy::updateElectricField(double dt, std::vector<std::unique_ptr<AllyInstance>> &allies,
                                       Renderer &renderer, EnemyInstance &self)
{
    if (!m_electricField.active)
        return;

    const double waveDuration = 0.3;
    const double waveGap = 0.2;
    const double maxRadius = 500.0;
    const int totalWaves = 3;

    // 间隔期
    if (m_electricField.gapTimer > 0.0)
    {
        m_electricField.gapTimer -= dt;
        if (m_electricField.gapTimer <= 0.0)
        {
            m_electricField.waveIndex++;
            m_electricField.waveTimer = 0.0;
            m_electricField.waveDamageDealt = false;
        }
        return;
    }

    // 波纹期
    m_electricField.waveTimer += dt;
    double progress = std::min(1.0, m_electricField.waveTimer / waveDuration);

    QEasingCurve curve(QEasingCurve::OutCubic);
    double easedProgress = curve.valueForProgress(progress);
    double currentRadius = maxRadius * (1.0 - easedProgress);

    // 法杖特效
    double rot = std::sin(4 * 2 * M_PI * m_electricField.waveTimer);
    renderer.queueImage(":/texture/projectile/maxwell/maxwell_1.png",
                        self.getX(), self.getY(), rot, 1.0, Qt::AlignCenter, 100);
    // 伤害（每波一次）
    if (!m_electricField.waveDamageDealt)
    {
        dealElectricDamage(allies, self,
                           m_electricField.centerX, m_electricField.centerY, currentRadius);
        m_electricField.waveDamageDealt = true;
    }

    // 吸引
    pullAlliesTowardCenter(allies, m_electricField.centerX, m_electricField.centerY, currentRadius, dt);

    // 特效
    renderElectricWave(renderer, m_electricField.centerX, m_electricField.centerY, currentRadius, m_electricField.waveTimer);

    // 波纹结束
    if (m_electricField.waveTimer >= waveDuration)
    {
        if (m_electricField.waveIndex >= totalWaves - 1)
        {
            m_electricField.reset();
            m_cooldown = 3.0;
            self.currentMp += 30.0; // 每次电场结束回复30MP
        }
        else
        {
            m_electricField.gapTimer = waveGap;
        }
    }
}

// 造成电击伤害
void PhysicsEnemy::dealElectricDamage(std::vector<std::unique_ptr<AllyInstance>> &allies,
                                      EnemyInstance &self,
                                      double centerX, double centerY,
                                      double radius)
{
    double baseDamage = self.atk.getFinal() * 0.5;

    for (const auto &ally : allies)
    {
        if (!ally->isAlive)
            continue;
        double dx = ally->transform.x - centerX;
        double dy = ally->transform.y - centerY;
        if (dx * dx + dy * dy < radius * radius)
        {
            ally->dealDamage(baseDamage, self,
                             DamageType{DamageType::Physical, QColor("#4488ff")});
        }
    }
}

// 吸引
void PhysicsEnemy::pullAlliesTowardCenter(std::vector<std::unique_ptr<AllyInstance>> &allies,
                                          double centerX, double centerY,
                                          double radius, double dt)
{
    const double pullStrength = 300.0;
    for (const auto &ally : allies)
    {
        if (!ally->isAlive || ally->isTower)
            continue;
        double dx = ally->transform.x - centerX;
        double dy = ally->transform.y - centerY;
        double dist = std::sqrt(dx * dx + dy * dy);
        if (dist < radius && dist > 10.0)
        {
            double force = (1.0 - dist / radius) * pullStrength * dt;
            ally->transform.x -= (dx / dist) * force;
            ally->transform.y -= (dy / dist) * force;
        }
    }
}

// 渲染
void PhysicsEnemy::renderElectricWave(Renderer &renderer,
                                      double centerX, double centerY,
                                      double radius, double waveTimer)
{
    if (radius <= 1.0)
        return;
    // 电波特效
    QPen pen;
    pen.setColor(QColor("#4488ff"));
    pen.setWidth(6);
    renderer.queueCircle(centerX, centerY, radius,
                         pen, QBrush(Qt::NoBrush), 100);
}
// % 旋度场
// 生成
void PhysicsEnemy::summonCurlField(EnemyInstance &self)
{
    // 能量满自动释放
    if (self.currentMp >= self.maxMp)
    {
        self.currentMp -= self.maxMp;
        m_curlField.reset();
        m_curlField.active = true;
        m_curlField.timer = 0.0;
    }
}

// 更新
void PhysicsEnemy::updateCurlField(double dt, std::vector<std::unique_ptr<AllyInstance>> &allies,
                                   std::vector<EnemyInstance *> &enemies,
                                   Renderer &renderer, EnemyInstance &self)
{

    m_curlField.timer += dt;
    double t = m_curlField.timer;
    double halfDuration = m_curlField.duration / 2.0; // 0.25s
    double rot = std::sin(4 * 2 * M_PI * t);
    // ---------- 旋转方向 ----------
    if (t <= halfDuration)
    {
        applyCurlRotation(allies, enemies, self, dt, true); // 顺时针

        renderer.queueImage(":/texture/projectile/maxwell/maxwell_3.png",
                            self.transform.x, self.transform.y,
                            rot, 2.0, Qt::AlignCenter, 80);
        // 前半段伤害（只触发一次）
        if (!m_curlField.firstHalfDamageDealt)
        {
            dealCurlDamage(allies, self);
            m_curlField.firstHalfDamageDealt = true;
        }
    }
    else if (t <= m_curlField.duration)
    {
        applyCurlRotation(allies, enemies, self, dt, false); // 逆时针
        renderer.queueImage(":/texture/projectile/maxwell/maxwell_4.png",
                            self.transform.x, self.transform.y,
                            rot, 2.0, Qt::AlignCenter, 80);
        // 后半段治疗（只触发一次）
        if (!m_curlField.secondHalfHealDealt)
        {
            dealCurlHeal(enemies, self, renderer);
            m_curlField.secondHalfHealDealt = true;
        }
    }
    else
    {
        // 结束
        m_curlField.reset();
    }

    // 特效
    renderCurlEffect(renderer, self);
}
// t ∈ [0, 1]
// InOutCubic 的导数
double df(double t)
{
    if (t < 0.5)
        return 12.0 * t * t; // 0 → 3（加速段）
    else
        return 12.0 * (1.0 - t) * (1.0 - t); // 3 → 0（减速段）
}
// 旋转
void PhysicsEnemy::applyCurlRotation(std::vector<std::unique_ptr<AllyInstance>> &allies,
                                     std::vector<EnemyInstance *> &enemies,
                                     EnemyInstance &self,
                                     double dt,
                                     bool clockwise)
{
    const double maxRadius = m_curlField.radius;
    const double baseAngularSpeed = 90.0;
    const double angleSign = clockwise ? 1.0 : -1.0;

    // 旋转单个目标
    auto rotateOne = [&](auto &target)
    {
        if (!target->isAlive)
            return;
        // 盟友额外检查塔
        if constexpr (std::is_same_v<std::decay_t<decltype(*target)>, AllyInstance>)
        {
            if (target->isTower)
                return;
        }

        double dx = target->transform.x - self.transform.x;
        double dy = target->transform.y - self.transform.y;
        double dist = std::sqrt(dx * dx + dy * dy);

        if (dist > maxRadius || dist < 1.0)
            return;

        // 转速随距离衰减
        double distanceFactor = 1.0 - (dist / maxRadius);

        // 使用缓动曲线让转速变化更自然
        double timeProgress = std::min(1.0, m_curlField.timer / m_curlField.duration);
        double timeFactor = df(timeProgress);

        double angleDelta = baseAngularSpeed * distanceFactor * timeFactor * dt * angleSign;
        double currentAngle = std::atan2(dy, dx);
        double newAngle = currentAngle + angleDelta * M_PI / 180.0;

        target->transform.x = self.transform.x + dist * std::cos(newAngle);
        target->transform.y = self.transform.y + dist * std::sin(newAngle);
    };

    // 根据方向选择目标
    if (clockwise)
    {
        for (auto &ally : allies)
            rotateOne(ally);
    }
    else
    {
        for (auto &enemy : enemies)
            rotateOne(enemy);
    }
}

// 彼之砒霜
void PhysicsEnemy::dealCurlDamage(std::vector<std::unique_ptr<AllyInstance>> &allies,
                                  EnemyInstance &self)
{
    const double maxRadius = m_curlField.radius;
    double baseDamage = self.atk.getFinal() * 0.8; // 伤害系数

    for (auto &ally : allies)
    {
        if (!ally->isAlive)
            continue;

        double dx = ally->transform.x - self.transform.x;
        double dy = ally->transform.y - self.transform.y;
        double dist = std::sqrt(dx * dx + dy * dy);

        if (dist < maxRadius)
        {
            // 越靠近中心伤害越高
            double dmgMultiplier = 1.0 + (1.0 - dist / maxRadius) * 0.5; // 1.0 ~ 1.5 倍
            ally->dealDamage(baseDamage * dmgMultiplier, self,
                             DamageType{DamageType::Physical, QColor("#8844ff")});
        }
    }
}

// 我之蜜糖
void PhysicsEnemy::dealCurlHeal(std::vector<EnemyInstance *> &enemies,
                                EnemyInstance &self, Renderer &renderer)
{
    const double maxRadius = m_curlField.radius;
    double baseHeal = self.atk.getFinal(); // 治疗系数

    for (auto &enemy : enemies)
    {
        if (!enemy->isAlive)
            continue;

        double dx = enemy->transform.x - self.transform.x;
        double dy = enemy->transform.y - self.transform.y;
        double dist = std::sqrt(dx * dx + dy * dy);

        if (dist < maxRadius)
        {
            // 越靠近中心治疗效果越好
            double healMultiplier = 1.0 + (1.0 - dist / maxRadius) * 0.5; // 1.0 ~ 1.5 倍
            enemy->currentHp += baseHeal * healMultiplier;
            renderer.queueSplash("+" + QString::number(std::ceil(baseHeal * healMultiplier)),
                                 enemy->transform.x, enemy->transform.y,
                                 Qt::green, 200);
        }
    }
}

// 渲染
void PhysicsEnemy::renderCurlEffect(Renderer &renderer, EnemyInstance &self)
{

    double t = m_curlField.timer;
    double halfDuration = m_curlField.duration / 2.0;

    // 主体光环：高速旋转
    double ringRotation = t * 720.0; // 每秒转两圈
    double ringScale = 2.0;          // 1000px 半径对应 scale = 1000/128 ≈ 7.8，这里用 queueImage 的 scale 参数放大

    // 实际 scale = 期望半径 / 纹理默认半径(64px)
    double scale = m_curlField.radius / 64.0;

    renderer.queueCircle(self.transform.x, self.transform.y, m_curlField.radius,
                         QPen(QColor("#8844ff"), 4), QBrush(Qt::NoBrush), 90);
}

// % 走位
void PhysicsEnemy::updatePos(double dt, EnemyInstance &self,
                             std::vector<std::unique_ptr<AllyInstance>> &allies)
{
    const double speed = self.speed * 100;
    const double minRetreatDist = 200.0;
    const double decentDist = 500.0;

    // 找最近非塔和塔
    AllyInstance *nearest = nullptr, *tower = nullptr;
    double minDist = std::numeric_limits<double>::max();

    for (auto &a : allies)
    {
        if (!a->isAlive)
            continue;
        if (a->isTower)
        {
            tower = a.get();
            continue;
        }
        double d = std::hypot(a->transform.x - self.transform.x, a->transform.y - self.transform.y);
        if (d < minDist)
        {
            minDist = d;
            nearest = a.get();
        }
    }

    AllyInstance *target = nullptr;
    bool retreat = false;

    if (nearest)
    {
        if (minDist < minRetreatDist)
        {
            target = nearest;
            retreat = true; // 太近，撤退
        }
        else if (minDist > decentDist)
        {
            target = nearest;
            retreat = false; // 太远，靠近
        }
        // 在 [minRetreatDist, decentDist] 区间不动
    }
    else if (tower)
    {
        target = tower; // 只剩塔，靠近
    }

    if (!target)
        return;

    double dx = target->transform.x - self.transform.x;
    double dy = target->transform.y - self.transform.y;
    if (retreat)
    {
        dx = -dx;
        dy = -dy;
    }

    double dist = std::hypot(dx, dy);
    if (dist < 0.001)
        return;

    self.transform.x += (dx / dist) * speed * dt;
    self.transform.y += (dy / dist) * speed * dt;
}

// % 格挡技
void PhysicsEnemy::checkHp(EnemyInstance &self, Renderer &renderer)
{
    if (m_lastRecordedHp > self.currentHp)
    {
        auto rng = QRandomGenerator::global();
        int rand = rng->bounded(5);
        if (rand == 0)
        { // 0.2概率触发格挡
            double damage = m_lastRecordedHp - self.currentHp;
            renderer.queueSplash(QString("+%1").arg(std::ceil(damage)),
                                 self.getX(), self.getY(), Qt::green);
            self.currentHp = m_lastRecordedHp; // 恢复到之前的血量
            m_blockingEffectTimer = 0.5;       // 格挡特效持续0.5秒
            return;
        }
        m_lastRecordedHp = self.currentHp;
    }
}

void PhysicsEnemy::updateBlockingEffect(double dt, EnemyInstance &self, Renderer &renderer)
{
    m_blockingEffectTimer = std::max(0.0, m_blockingEffectTimer - dt);
    if (m_blockingEffectTimer <= 0.0)
        return;

    double scale = 0.5;
    if (m_blockingEffectTimer >= 0.4)
    {
        // 跳入
        QEasingCurve curve(QEasingCurve::OutCubic);
        double progress = curve.valueForProgress(1 - (m_blockingEffectTimer - 0.4) / 0.1);
        scale = 0.5 + 0.5 * progress; // 从0.5增长到1.5
    }
    else
    {
        scale = 1.0; // 保持在1.0
    }
    // 显示图片
    renderer.queueImage(":/texture/projectile/maxwell/maxwell_2.png", self.getX(), self.getY(), 0, scale, Qt::AlignCenter, 150);
}
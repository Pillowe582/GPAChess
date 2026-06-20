#include "mainwindow.h"
#include "print.h"
#include "ui_entry.h"
#include "logo_player.h"
#include "game_manager.h"
#include "unit_graphics_item.h"
#include "database_manager.h"
#include "shop_window.h"

#include <QBrush>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QGraphicsEllipseItem>
#include <QGraphicsPathItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsView>
#include <QLabel>
#include <QMessageBox>
#include <QPainterPath>
#include <QPen>
#include <QPushButton>
#include <QSet>
#include <QList>
#include <QTimer>
#include <QVariantAnimation>
#include <algorithm>
#include <cmath>

// ============================================================================
// % 程序生命周期
// ============================================================================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    initUi();
    print("Entry point initialized");

    // —— Logo 播放 ——
    LogoPlayer *logoPlayer = new LogoPlayer(ui->logoDisplayLabel, this);
    connect(logoPlayer, &LogoPlayer::logoPlayFinished, this, [this]()
            { this->switchScene(Scene::EntryMenu); });

    // —— 入口界面按钮 ——
    connect(ui->startGameButton, &QPushButton::clicked, this, &MainWindow::initGame);
    connect(ui->settingsButton, &QPushButton::clicked, this, [this]()
            { print("Opening settings"); }); // TODO
}

/// @brief UI 初始化：固定窗口尺寸、切到 Logo 场景、搭建战场
void MainWindow::initUi()
{
    ui->sceneController->setCurrentIndex(Scene::EntryLogo);
    this->setFixedSize(1920, 1080);
    this->setWindowTitle("GPAutoChess");
    setupGameScene();
}

/// @brief 首次点击"开始游戏"触发 —— 创建核心对象并绑定所有信号槽
void MainWindow::initGame()
{
    print("Starting game");
    switchScene(Scene::MainGame);

    if (!m_gameManager)
    {
        // —— 创建核心对象 ——
        m_gameManager = new GameManager(this);
        if (!m_database)
            m_database = new DatabaseManager(
                QCoreApplication::applicationDirPath() + "/assets/entities", this);
        m_gameManager->setDatabase(m_database);

        // —— GameManager → MainWindow ——
        connect(m_gameManager, &GameManager::splashText,
                this, &MainWindow::showSplashText);
        connect(m_gameManager, &GameManager::phaseChanged, this, [this](RoundPhase p)
                {
            print(QString("Phase changed: %1").arg(static_cast<int>(p)));
            ui->startRoundButton->setEnabled(p == RoundPhase::Prepare);
            ui->openShopButton->setEnabled(p == RoundPhase::Prepare);
            refreshSceneLabels();
            refreshAllUnits(); });
        connect(m_gameManager, &GameManager::tick, this, [this]()
                {
            refreshBattleGround();
            refreshAllUnits(); });
        connect(m_gameManager, &GameManager::roundEnded,
                this, &MainWindow::showRoundResult);
        connect(m_gameManager, &GameManager::gameOver,
                this, &MainWindow::showGameOver);

        // —— UI 按钮 → GameManager ——
        connect(ui->startRoundButton, &QPushButton::clicked, this, [this]()
                {
            if (!m_gameManager) return;
            m_gameManager->startRound(m_gameManager->getRoundNumber());
            refreshBattleGround();
            refreshAllUnits(); });
        connect(ui->openShopButton, &QPushButton::clicked,
                this, &MainWindow::onShopOpenClicked);
    }

    m_gameManager->initialize();
    refreshAllUnits();
    refreshSceneLabels();
}

MainWindow::~MainWindow()
{
    print("Mainba out");
    delete ui;
}

// ============================================================================
// % UI切换
// ============================================================================

/// @brief 切换场景（QStackedWidget 页面跳转）
int MainWindow::switchScene(Scene scene)
{
    print("Switching to scene " + QString::number(scene));
    int currentScene = ui->sceneController->currentIndex();
    ui->sceneController->setCurrentIndex(scene);
    return currentScene;
}

/// @brief 资源路径辅助：可执行文件目录 → assets/ 子目录
QString MainWindow::assetPath(const QString &relativePath) const
{
    const QString base = QCoreApplication::applicationDirPath();
    const QString candidate = QDir(base).filePath(QString("assets/%1").arg(relativePath));
    if (QFileInfo::exists(candidate))
        return candidate;
    print("未找到资源 " + candidate);
    return QString("assets/%1").arg(relativePath);
}

// ============================================================================
// % 战场绘制
// ============================================================================
/// @brief 创建 QGraphicsScene 并绘制所有静态元素（背景、塔、备战席、出售区等）
void MainWindow::setupGameScene()
{
    QGraphicsView *battleView = ui->mainBattleGround;
    if (!battleView)
    {
        print("Error: mainBattleGround is null");
        return;
    }

    // —— 场景创建 ——
    m_battleScene = new QGraphicsScene(this);
    battleView->setScene(m_battleScene);
    battleView->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    battleView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    battleView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    const int totalHeight = kBattlefieldHeight + kBenchAreaHeight;
    m_battleScene->setSceneRect(0, 0, kBattlefieldWidth, totalHeight);

    // —— 战场背景贴图 ——
    const QString bgPath = assetPath("battleground/sand.png");
    QPixmap bg(bgPath);
    if (!bg.isNull())
    {
        QSize sceneSize(kBattlefieldWidth, kBattlefieldHeight);
        const int safeMargin = 100;
        QSize targetSize(sceneSize.width() + safeMargin * 2,
                         sceneSize.height() + safeMargin * 2);
        auto scaled = bg.scaled(targetSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        auto *bgItem = m_battleScene->addPixmap(scaled);
        bgItem->setZValue(-1145);
        int offsetX = (sceneSize.width() - scaled.width()) / 2;
        int offsetY = (sceneSize.height() - scaled.height()) / 2;
        bgItem->setPos(offsetX, offsetY);
    }
    else
    {
        m_battleScene->setBackgroundBrush(QBrush(QColor("#424242")));
    }

    // —— 塔占位符 ——
    const double towerSize = 70.0;
    const double towerX = 10.0;
    const double towerY = kBattlefieldHeight / 2.0 - towerSize / 2.0;
    auto *tower = m_battleScene->addRect(
        QRectF(towerX, towerY, towerSize, towerSize),
        QPen(QColor(80, 160, 255), 3.0),
        QBrush(QColor(40, 100, 220, 200)));
    tower->setZValue(5);

    m_towerGpaText = m_battleScene->addSimpleText(QStringLiteral("4.0"));
    m_towerGpaText->setBrush(Qt::black);
    QFont gpaFont("Microsoft YaHei", 14);
    gpaFont.setBold(true);
    m_towerGpaText->setFont(gpaFont);
    m_towerGpaText->setZValue(7);

    // —— 备战席区域背景 ——
    const int horizontalMargin = 50;
    const double benchY = static_cast<double>(kBattlefieldHeight);
    auto *benchBg = m_battleScene->addRect(
        QRectF(horizontalMargin, benchY, kBattlefieldWidth - 2 * horizontalMargin, kBenchAreaHeight),
        QPen(Qt::NoPen), QBrush(QColor("#686868")));
    benchBg->setZValue(-10);

    // —— 部署区提示边框 ——
    const double deployLeft = kBattlefieldMargin;
    const double deployTop = kBattlefieldMargin;
    const double deployRight = kBattlefieldWidth / 2.0 - kBattlefieldMargin + kDeployRightExtension;
    const double deployBottom = kBattlefieldHeight - kBattlefieldMargin;
    const double deployRadius = 20.0;

    QPainterPath deployPath;
    deployPath.addRoundedRect(
        QRectF(deployLeft, deployTop, deployRight - deployLeft, deployBottom - deployTop),
        deployRadius, deployRadius);
    auto *deployBorder = m_battleScene->addPath(deployPath,
                                                QPen(QColor("#00ddff") ^ 0.7, 3.0),
                                                QBrush(QColor("#00ddff") ^ 0.1));
    deployBorder->setZValue(-5);

    // —— 备战席槽位 ——
    for (int i = 0; i < PlayerAssets::maxBench; ++i)
    {
        const double x = kBenchSlotStartX + i * kBenchSlotWidth;
        const double y = kBenchSlotStartY;
        auto *slot = m_battleScene->addRect(
            QRectF(x, y, kBenchSlotWidth - 8, kBenchSlotHeight),
            QPen(QColor("#a0a0a0"), 2),
            QBrush(QColor("#323232") ^ 0.5));
        slot->setZValue(1);
    }

    // —— 出售区 ——
    const double sellSize = 100.0;
    const double sellX = kBenchSlotStartX + PlayerAssets::maxBench * kBenchSlotWidth + 20.0;
    const double sellY = kBenchSlotStartY + (kBenchSlotHeight - sellSize) / 2.0;
    auto *sellZone = m_battleScene->addRect(
        QRectF(sellX, sellY, sellSize, sellSize),
        QPen(QColor("#ff5050"), 3.0),
        QBrush(QColor("#c82828") ^ 0.7));
    sellZone->setZValue(1);
    m_sellLabel = m_battleScene->addSimpleText(QStringLiteral("出售"));
    m_sellLabel->setBrush(Qt::white);
    QFont sellFont("Microsoft YaHei", 12);
    sellFont.setBold(true);
    m_sellLabel->setFont(sellFont);
    m_sellLabel->setPos(sellX + sellSize / 2.0 - m_sellLabel->boundingRect().width() / 2.0,
                        sellY + sellSize / 2.0 - m_sellLabel->boundingRect().height() / 2.0);
    m_sellLabel->setZValue(2);
}

// ============================================================================
// % 战场刷新
// ============================================================================

/// @brief 刷新敌方单位 + behavior 产生的 DrawCmd（子弹、武器挥砍等）
void MainWindow::refreshBattleGround()
{
    if (!m_battleScene || !m_gameManager)
        return;

    // ——— 清除上一帧的临时图形项 ———
    QList<QGraphicsItem *> allItems = m_battleScene->items();
    for (auto *item : allItems)
    {
        if (item->data(0).toInt() == -1)
        {
            m_battleScene->removeItem(item);
            delete item;
        }
    }

    // ——— 绘制敌方单位 ———
    const auto &enemies = m_gameManager->getEnemies();
    const double lerpT = m_gameManager->getTickLerp();
    for (size_t i = 0; i < enemies.size(); ++i)
    {
        const EnemyInstance &enemy = enemies[i];
        if (!enemy.isAlive)
            continue;

        // tick 间视觉插值
        const double x = enemy.prevPosX + (enemy.posX - enemy.prevPosX) * lerpT;
        const double y = enemy.prevPosY + (enemy.posY - enemy.prevPosY) * lerpT;
        const double radius = 34.0;
        const QColor color(240, 70, 70);

        // 身体
        QRectF bodyRect(x - radius, y - radius, radius * 2.0, radius * 2.0);
        auto *body = m_battleScene->addEllipse(bodyRect, QPen(Qt::black, 2), QBrush(color));
        body->setData(0, -1);
        body->setZValue(10);

        // 血条背景
        const double barWidth = radius * 3.0;
        const double barHeight = 8.0;
        const double barX = x - barWidth / 2.0;
        const double barY = y - radius - 24.0;
        auto *barBack = m_battleScene->addRect(
            QRectF(barX, barY, barWidth, barHeight),
            QPen(Qt::NoPen), QBrush(QColor(40, 40, 40, 220)));
        barBack->setData(0, -1);
        barBack->setZValue(20);

        // 血条前景
        const int maxHp = enemy.hp.getFinal();
        const double hpRatio = maxHp > 0
                                   ? std::clamp(static_cast<double>(enemy.currentHp) / static_cast<double>(maxHp), 0.0, 1.0)
                                   : 0.0;
        auto *barFront = m_battleScene->addRect(
            QRectF(barX, barY, barWidth * hpRatio, barHeight),
            QPen(Qt::NoPen), QBrush(QColor(220, 60, 60)));
        barFront->setData(0, -1);
        barFront->setZValue(21);

        // 名字 / 血量（阴影 + 前景）
        auto *textShadow = m_battleScene->addSimpleText(
            QString("%1\n%2/%3").arg(enemy.name).arg(enemy.currentHp).arg(maxHp));
        textShadow->setBrush(QColor(0, 0, 0, 160));
        textShadow->setData(0, -1);
        textShadow->setZValue(29);
        textShadow->setPos(x - textShadow->boundingRect().width() / 2.0 + 1.0,
                           barY + barHeight + 3.0);

        auto *text = m_battleScene->addSimpleText(
            QString("%1\n%2/%3").arg(enemy.name).arg(enemy.currentHp).arg(maxHp));
        text->setBrush(Qt::white);
        text->setData(0, -1);
        text->setZValue(30);
        text->setPos(x - text->boundingRect().width() / 2.0, barY + barHeight + 2.0);
    }

    // ——— 渲染 behavior 产生的 DrawCmd ———
    for (const auto &cmd : m_gameManager->getPendingDraws())
    {
        if (cmd.kind == DrawCmd::Circle)
        {
            double r = cmd.param1;
            auto *circle = m_battleScene->addEllipse(
                cmd.x - r, cmd.y - r, r * 2.0, r * 2.0,
                QPen(Qt::NoPen), QBrush(cmd.color));
            circle->setData(0, -1);
            circle->setZValue(cmd.zValue);
        }
        else if (cmd.kind == DrawCmd::RotatedRect)
        {
            double hw = cmd.param1;
            double hh = cmd.param2;
            auto *rect = m_battleScene->addRect(
                QRectF(-hw, -hh, hw * 2.0, hh * 2.0),
                QPen(Qt::NoPen), QBrush(cmd.color));
            rect->setPos(cmd.x, cmd.y);
            rect->setRotation(cmd.angle * 180.0 / 3.14159265);
            rect->setData(0, -1);
            rect->setZValue(cmd.zValue);
        }
    }

    // ——— 更新塔上的学分绩 ———
    if (m_towerGpaText && m_gameManager)
    {
        double gpa = m_gameManager->getRoundGpa();
        m_towerGpaText->setText(QString::number(gpa, 'f', 1));
        const double tx = 10.0, ts = 70.0;
        const double ty = kBattlefieldHeight / 2.0 - ts / 2.0;
        m_towerGpaText->setPos(
            tx + ts / 2.0 - m_towerGpaText->boundingRect().width() / 2.0,
            ty + ts / 2.0 - m_towerGpaText->boundingRect().height() / 2.0);
    }
}

/// @brief 刷新我方单位（部署 → 用 posX/posY；备战 → 按槽位算坐标）
void MainWindow::refreshAllUnits()
{
    if (!m_battleScene || !m_gameManager)
        return;

    const auto &units = m_gameManager->getPlayerAssets().ownedChesses;
    QSet<int> currentUuids;
    const bool isPrepare = (m_gameManager->getCurrentPhase() == RoundPhase::Prepare);
    const double lerpT = m_gameManager->getTickLerp();

    for (const auto &unit : units)
    {
        currentUuids.insert(unit.uuid);
        UnitGraphicsItem *item = m_unitItems.value(unit.uuid, nullptr);

        if (!item)
        {
            item = new UnitGraphicsItem(unit.uuid);
            m_battleScene->addItem(item);
            m_unitItems[unit.uuid] = item;
            connect(item, &UnitGraphicsItem::dragStarted,
                    this, &MainWindow::onUnitDragStarted);
            connect(item, &UnitGraphicsItem::dragFinished,
                    this, &MainWindow::onUnitDragFinished);
        }

        QPointF targetPos;
        double radius;
        QColor color;

        if (unit.deployed)
        {
            // tick 间视觉插值
            const double lx = unit.prevPosX + (unit.posX - unit.prevPosX) * lerpT;
            const double ly = unit.prevPosY + (unit.posY - unit.prevPosY) * lerpT;
            targetPos = QPointF(lx, ly);
            radius = 34.0;
            color = QColor(60, 110, 255);
        }
        else
        {
            int slot = unit.benchSlot;
            if (slot < 0 || slot >= PlayerAssets::maxBench)
                slot = 0;
            const double cx = kBenchSlotStartX + slot * kBenchSlotWidth + (kBenchSlotWidth - 8) / 2.0;
            const double cy = kBenchSlotStartY + kBenchSlotHeight / 2.0;
            targetPos = QPointF(cx, cy);
            radius = 22.0;
            color = QColor(70, 120, 255);
        }

        item->setVisible(unit.isAlive);
        item->setPos(targetPos);
        item->setDraggable(isPrepare);
        item->updateVisual(unit.name, unit.currentHp, unit.hp.getFinal(),
                           color, radius, unit.starLevel, unit.deployed);
    }

    // ——— 清理已不存在的单位图形项 ———
    QList<int> toRemove;
    for (auto it = m_unitItems.begin(); it != m_unitItems.end(); ++it)
    {
        if (!currentUuids.contains(it.key()))
            toRemove.append(it.key());
    }
    for (int uuid : toRemove)
        delete m_unitItems.take(uuid);
}

/// @brief 战斗跳字（伤害/回血等）
void MainWindow::showSplashText(const QString &text, double x, double y, const QString &color)
{
    if (!m_battleScene)
        return;

    QFont ftFont("Microsoft YaHei", 20);
    ftFont.setBold(true);

    // 前景
    auto *font = m_battleScene->addSimpleText(text);
    font->setFont(ftFont);
    font->setBrush(QColor(color));
    font->setPos(x - font->boundingRect().width() / 2.0, y - 30.0);
    font->setTransformOriginPoint(font->boundingRect().center());
    font->setZValue(500);
    font->setScale(4.0);

    // 阴影
    auto *shadow = m_battleScene->addSimpleText(text);
    shadow->setFont(ftFont);
    shadow->setBrush(QColor("#000000") ^ 0.6);
    shadow->setPos(font->pos() + QPointF(2, 2));
    shadow->setTransformOriginPoint(shadow->boundingRect().center());
    shadow->setZValue(499);
    shadow->setScale(4.0);

    // 缩放动画 4x → 1x, 300ms
    auto *anim = new QVariantAnimation(this);
    anim->setDuration(300);
    anim->setStartValue(4.0);
    anim->setEndValue(1.0);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    connect(anim, &QVariantAnimation::valueChanged, this,
            [font, shadow](const QVariant &val)
            {
                double s = val.toDouble();
                font->setScale(s);
                shadow->setScale(s);
            });
    connect(anim, &QVariantAnimation::finished, anim, &QObject::deleteLater);
    anim->start(QAbstractAnimation::DeleteWhenStopped);

    // 800ms 后自动移除
    QTimer::singleShot(800, this, [font, shadow]()
                       {
        if (font->scene())   font->scene()->removeItem(font);
        if (shadow->scene()) shadow->scene()->removeItem(shadow);
        delete font;
        delete shadow; });
}

// ============================================================================
// % 拖拽交互
// ============================================================================

/// @brief 将出售标签重新居中（用于文字变化后修正位置）
void MainWindow::recenterSellLabel()
{
    if (!m_sellLabel)
        return;
    const double sellX = kBenchSlotStartX + PlayerAssets::maxBench * kBenchSlotWidth + 20.0;
    const double sellSize = 100.0;
    const double sellY = kBenchSlotStartY + (kBenchSlotHeight - sellSize) / 2.0;
    m_sellLabel->setPos(sellX + sellSize / 2.0 - m_sellLabel->boundingRect().width() / 2.0,
                        sellY + sellSize / 2.0 - m_sellLabel->boundingRect().height() / 2.0);
}

/// @brief 拖拽开始时的处理
void MainWindow::onUnitDragStarted(int uuid, QPointF scenePos)
{
    if (!m_gameManager || !m_sellLabel)
        return;

    const auto *unit = m_gameManager->getPlayerAssets().getUnitByUuid(uuid);
    if (!unit)
        return;

    int worth = m_gameManager->getTotalWorth(unit->starLevel, unit->cost);
    m_sellLabel->setText(QString("出售 ￥%1").arg(worth));
    recenterSellLabel();
}

/// @brief 判断坐标是否在部署区（左半场）
bool MainWindow::isInBattlefieldLegalZone(QPointF scenePos) const
{
    const double x = scenePos.x();
    const double y = scenePos.y();
    const double deployRight = kBattlefieldWidth / 2.0 - kBattlefieldMargin + kDeployRightExtension;
    return x >= kBattlefieldMargin && x <= deployRight && y >= kBattlefieldMargin && y <= kBattlefieldHeight - kBattlefieldMargin;
}

/// @brief 判断坐标是否在备战席区域
bool MainWindow::isInBenchZone(QPointF scenePos) const
{
    const double y = scenePos.y();
    return y > static_cast<double>(kBattlefieldHeight) && y < static_cast<double>(kBattlefieldHeight + kBenchAreaHeight);
}

/// @brief 判断坐标是否在出售区（红色方块）
bool MainWindow::isInSellZone(QPointF scenePos) const
{
    const double sellX = kBenchSlotStartX + PlayerAssets::maxBench * kBenchSlotWidth + 20.0;
    const double sellY = kBenchSlotStartY + (kBenchSlotHeight - 100.0) / 2.0;
    const double sellSize = 100.0;
    return scenePos.x() >= sellX && scenePos.x() <= sellX + sellSize && scenePos.y() >= sellY && scenePos.y() <= sellY + sellSize;
}

/// @brief 找到离触摸点最近的空备战席槽位
/// @return 槽位索引，-1 表示已满
int MainWindow::nearestBenchSlot(QPointF scenePos) const
{
    if (!m_gameManager)
        return -1;
    const auto &units = m_gameManager->getPlayerAssets().ownedChesses;

    // 标记已占用槽位
    bool occupied[PlayerAssets::maxBench] = {};
    for (const auto &u : units)
        if (!u.deployed && u.benchSlot >= 0 && u.benchSlot < PlayerAssets::maxBench)
            occupied[u.benchSlot] = true;

    const double x = scenePos.x();
    int bestSlot = -1;
    double bestDist = 1e18;
    for (int i = 0; i < PlayerAssets::maxBench; ++i)
    {
        if (occupied[i])
            continue;
        const double slotCx = kBenchSlotStartX + i * kBenchSlotWidth + (kBenchSlotWidth - 8) / 2.0;
        const double dist = std::abs(x - slotCx);
        if (dist < bestDist)
        {
            bestDist = dist;
            bestSlot = i;
        }
    }
    return bestSlot;
}

/// @brief 拖拽松手后的统一处理
///
/// 分支：
///   S → 出售区 → sellUnit
///   A → 备战席 → 战场 → deploy
///   B → 战场 → 备战席 → undeploy
///   C → 战场内移动 → 更新 posX / posY
///   D → 备战席内交换 / 移动
///   E → 非法区域 → 弹回（不做修改）
void MainWindow::onUnitDragFinished(int uuid, QPointF scenePos)
{
    if (!m_gameManager)
        return;

    // 非准备阶段不允许拖拽
    if (m_gameManager->getCurrentPhase() != RoundPhase::Prepare)
    {
        refreshAllUnits();
        return;
    }

    auto &assets = m_gameManager->getPlayerAssets();
    auto &units = assets.ownedChesses;

    // 查找拖拽的单位
    ChessInstance *unit = assets.getUnitByUuid(uuid);
    if (!unit)
    {
        m_sellLabel->setText(QStringLiteral("出售"));
        recenterSellLabel();
        refreshAllUnits();
        return;
    }

    const bool wasDeployed = unit->deployed;
    const int oldBenchSlot = unit->benchSlot;

    // ---- S: 出售 ----
    if (isInSellZone(scenePos))
    {
        m_gameManager->sellUnit(uuid);
        m_sellLabel->setText(QStringLiteral("出售"));
        recenterSellLabel();
        refreshAllUnits();
        refreshSceneLabels();
        return;
    }

    // ---- A: 备战席 → 战场 ----
    if (!wasDeployed && isInBattlefieldLegalZone(scenePos))
    {
        if (!assets.battlefieldFull())
        {
            unit->deployed = true;
            unit->benchSlot = -1;
            unit->posX = scenePos.x();
            unit->posY = scenePos.y();
            unit->prevPosX = unit->posX;
            unit->prevPosY = unit->posY;
            print(QString("Deploy unit %1 to battlefield (%2,%3)")
                      .arg(uuid)
                      .arg(scenePos.x())
                      .arg(scenePos.y()));
        }
    }
    // ---- B: 战场 → 备战席 ----
    else if (wasDeployed && isInBenchZone(scenePos))
    {
        int targetSlot = nearestBenchSlot(scenePos);
        if (targetSlot >= 0)
        {
            unit->deployed = false;
            unit->benchSlot = targetSlot;
            unit->posX = 0;
            unit->posY = 0;
            print(QString("Undeploy unit %1 to bench slot %2").arg(uuid).arg(targetSlot));
        }
    }
    // ---- C: 战场内移动 ----
    else if (wasDeployed && isInBattlefieldLegalZone(scenePos))
    {
        unit->posX = scenePos.x();
        unit->posY = scenePos.y();
        print(QString("Move unit %1 on battlefield to (%2,%3)")
                  .arg(uuid)
                  .arg(scenePos.x())
                  .arg(scenePos.y()));
    }
    // ---- D: 备战席内交换 / 移动 ----
    else if (!wasDeployed && isInBenchZone(scenePos))
    {
        int rawSlot = -1;
        double bestDist = 1e18;
        const double sx = scenePos.x();
        for (int i = 0; i < PlayerAssets::maxBench; ++i)
        {
            const double slotCx = kBenchSlotStartX + i * kBenchSlotWidth + (kBenchSlotWidth - 8) / 2.0;
            const double dist = std::abs(sx - slotCx);
            if (dist < bestDist)
            {
                bestDist = dist;
                rawSlot = i;
            }
        }

        if (rawSlot >= 0 && rawSlot != oldBenchSlot)
        {
            ChessInstance *occupant = nullptr;
            for (auto &u : units)
            {
                if (!u.deployed && u.benchSlot == rawSlot && u.uuid != uuid)
                {
                    occupant = &u;
                    break;
                }
            }
            if (occupant)
            {
                occupant->benchSlot = oldBenchSlot;
                unit->benchSlot = rawSlot;
                print(QString("Swap bench slots: %1 ↔ %2").arg(uuid).arg(occupant->uuid));
            }
            else
            {
                unit->benchSlot = rawSlot;
                print(QString("Move unit %1 to bench slot %2").arg(uuid).arg(rawSlot));
            }
        }
    }
    // ---- E: 非法区域 → 弹回 ----

    m_sellLabel->setText(QStringLiteral("出售"));
    recenterSellLabel();
    refreshAllUnits();
}

// ============================================================================
// % 战场上方标签
// ============================================================================

/// @brief 根据当前 GameManager 状态更新所有 UI 标签
void MainWindow::refreshSceneLabels()
{
    if (!m_gameManager)
        return;

    int round = m_gameManager->getRoundNumber();
    int credit = GameManager::getRoundCredit(round);
    const auto &assets = m_gameManager->getPlayerAssets();

    if (auto *lbl = findChild<QLabel *>("roundCount"))
        lbl->setText(QString("第%1门：XXX, %2cr").arg(round).arg(credit));
    if (auto *lbl = findChild<QLabel *>("totalCredit"))
        lbl->setText(QString("已修学分：%1").arg(m_gameManager->getPreviousCredits()));
    if (auto *lbl = findChild<QLabel *>("gpa"))
        lbl->setText(QString("GPA：%1/4.0").arg(m_gameManager->getAverageGpa(), 0, 'f', 2));
    if (auto *lbl = findChild<QLabel *>("roundValue"))
        lbl->setText(QString("上场：%1/%2").arg(assets.deployedCount()).arg(PlayerAssets::maxBattlefield));
    if (auto *lbl = findChild<QLabel *>("goldCount"))
        lbl->setText(QString("金币：%1").arg(assets.gold));
}

/// @brief 打开商店窗口（仅在准备阶段可用）
void MainWindow::onShopOpenClicked()
{
    if (!m_gameManager || !m_database)
        return;
    if (m_gameManager->getCurrentPhase() != RoundPhase::Prepare)
        return;

    if (!m_shopWindow)
    {
        m_shopWindow = new ShopWindow(m_database, m_gameManager, this);
        connect(m_shopWindow, &ShopWindow::shopClosed, this, [this]()
                {
            m_gameManager->checkAndMergeStars();
            refreshAllUnits();
            refreshSceneLabels(); });
    }
    m_shopWindow->refreshShop();
    m_shopWindow->show();
}

// ============================================================================
// % 结算
// ============================================================================

/// @brief 展示回合结算对话框
void MainWindow::showRoundResult(bool victory)
{
    if (!m_gameManager)
        return;

    print(QString("Round ended. Victory=%1").arg(victory));
    refreshSceneLabels();
    refreshAllUnits();

    const int goldEarned = m_gameManager->getRoundGoldEarned();
    const int expEarned = m_gameManager->getRoundExpEarned();
    const int towerHp = m_gameManager->getTowerHp();
    const double gpa = m_gameManager->getRoundGpa();

    QMessageBox box(this);
    box.setWindowTitle(QStringLiteral("回合结算"));
    box.setIcon(victory ? QMessageBox::Information : QMessageBox::Warning);
    box.setText(QString(
                    "%1\n\n"
                    "获得金币：%2\n"
                    "获得经验：%3\n"
                    "塔剩余血量：%4 / %5\n"
                    "本回合学分绩：%6 / 4.0（%7学分）")
                    .arg(victory ? QStringLiteral("✨ 胜利！") : QStringLiteral("💀 失败..."))
                    .arg(goldEarned)
                    .arg(expEarned)
                    .arg(towerHp)
                    .arg(m_gameManager->getMaxTowerHp())
                    .arg(gpa, 0, 'f', 2)
                    .arg(GameManager::getRoundCredit(m_gameManager->getRoundNumber())));
    box.setStandardButtons(QMessageBox::Ok);
    box.exec();

    QTimer::singleShot(300, this, [this]()
                       {
        if (m_gameManager)
            m_gameManager->nextRound(); });
}

/// @brief 展示整局游戏结算对话框
void MainWindow::showGameOver(double finalGpa)
{
    QMessageBox box(this);
    box.setWindowTitle(QStringLiteral("游戏结算"));
    box.setIcon(QMessageBox::Information);
    box.setText(QString("🎓 本学期结束！\n\nGPA：%1").arg(finalGpa, 0, 'f', 2));
    box.setStandardButtons(QMessageBox::Ok);
    box.exec();
    switchScene(Scene::EntryMenu);
}

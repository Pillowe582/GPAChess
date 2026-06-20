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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    initUi();
    print("Entry point initialized");

    LogoPlayer *logoPlayer = new LogoPlayer(ui->logoDisplayLabel, this);
    connect(logoPlayer, &LogoPlayer::logoPlayFinished, this, [this]()
            { this->switchScene(Scene::EntryMenu); }); // Logo 播放完毕后切换到菜单界面

    connect(ui->startGameButton, &QPushButton::clicked, this, &MainWindow::initGame); // 初始化游戏

    connect(ui->settingsButton, &QPushButton::clicked, this, [this]()
            { print("Opening settings"); }); // 设置按钮点击事件（TODO）
}

/// @brief 切换场景
/// @param scene 要切换到的场景，为int索引
/// @return 返回切换前的场景索引
int MainWindow::switchScene(Scene scene)
{
    print("Switching to scene " + QString::number(scene));
    int currentScene = ui->sceneController->currentIndex();
    ui->sceneController->setCurrentIndex(scene);
    return currentScene;
}

/// @brief UI初始化
void MainWindow::initUi()
{
    ui->sceneController->setCurrentIndex(Scene::EntryLogo);
    this->setFixedSize(1920, 1080);
    this->setWindowTitle("GPAutoChess");
    setupGameScene();
}

/// @brief 游戏初始化（只是在前面加个assets/）
/// @param relativePath 相对于可执行文件所在目录的资源路径（不带 "assets/" 前缀）
/// @return 资源的绝对路径，如果 assets/ 目录下不存在该资源，则返回相对路径（可能导致加载失败）
QString MainWindow::assetPath(const QString &relativePath) const
{
    const QString base = QCoreApplication::applicationDirPath();
    const QString candidate = QDir(base).filePath(QString("assets/%1").arg(relativePath));
    if (QFileInfo::exists(candidate))
        return candidate;
    print("未找到资源 " + candidate);
    return QString("assets/%1").arg(relativePath);
}

/// @brief 初始化游戏战场
void MainWindow::setupGameScene()
{
    QGraphicsView *battleView = ui->mainBattleGround;
    if (!battleView)
    {
        print("Error: mainBattleGround is null");
        return;
    }

    m_battleScene = new QGraphicsScene(this); // 注：QGraphicsScene是blablaView的一部分，专管场景显示
    battleView->setScene(m_battleScene);
    battleView->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    battleView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    battleView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    const int totalHeight = kBattlefieldHeight + kBenchAreaHeight;
    m_battleScene->setSceneRect(0, 0, kBattlefieldWidth, totalHeight);

    // 战场背景
    const QString bgPath = assetPath("battleground/sand.png");
    QPixmap bg(bgPath);
    if (!bg.isNull())
    {
        QSize sceneSize(kBattlefieldWidth, kBattlefieldHeight);
        const int safeMargin = 100;
        QSize targetSize(sceneSize.width() + safeMargin * 2,
                         sceneSize.height() + safeMargin * 2); // 稍微把背景放大些，预留空间
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

    //  塔占位符（TODO）
    const double towerSize = 70.0;
    const double towerX = 10.0;
    const double towerY = kBattlefieldHeight / 2.0 - towerSize / 2.0;
    auto *tower = m_battleScene->addRect(
        QRectF(towerX, towerY, towerSize, towerSize),
        QPen(QColor(80, 160, 255), 3.0),
        QBrush(QColor(40, 100, 220, 200)));
    tower->setZValue(5);

    // 备战席区域背景
    const int horizontalMargin = 50;
    const double benchY = static_cast<double>(kBattlefieldHeight);
    auto *benchBg = m_battleScene->addRect(
        QRectF(horizontalMargin, benchY, kBattlefieldWidth - 2 * horizontalMargin, kBenchAreaHeight),
        QPen(Qt::NoPen), QBrush(QColor("#686868")));
    benchBg->setZValue(-10);

    // 部署区淡蓝色边框提示
    const double deployLeft = kBattlefieldMargin;
    const double deployTop = kBattlefieldMargin;
    const double deployRight = kBattlefieldWidth / 2.0 - kBattlefieldMargin + kDeployRightExtension;
    const double deployBottom = kBattlefieldHeight - kBattlefieldMargin;
    const double deployRadius = 20.0; // 圆角半径 20px

    QPainterPath deployPath;
    deployPath.addRoundedRect(
        QRectF(deployLeft, deployTop,
               deployRight - deployLeft, deployBottom - deployTop),
        deployRadius, deployRadius);
    auto *deployBorder = m_battleScene->addPath(deployPath,
                                                QPen(QColor("#00ddff") ^ 0.7, 3.0),
                                                QBrush(QColor("#00ddff") ^ 0.1));
    deployBorder->setZValue(-5);

    // 备战席标签
    auto *benchLabel = m_battleScene->addSimpleText(QStringLiteral("备战席"));
    benchLabel->setBrush(QColor("#ffffff"));
    QFont labelFont("Microsoft YaHei", 14);
    labelFont.setBold(true);
    benchLabel->setFont(labelFont);
    benchLabel->setPos(10.0, benchY + 4.0);
    benchLabel->setZValue(1);

    // 备战席席位
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

    // 出售区
    const double sellSize = 100.0;
    const double sellX = kBenchSlotStartX + PlayerAssets::maxBench * kBenchSlotWidth + 20.0;
    const double sellY = kBenchSlotStartY + (kBenchSlotHeight - sellSize) / 2.0;
    auto *sellZone = m_battleScene->addRect(
        QRectF(sellX, sellY, sellSize, sellSize),
        QPen(QColor("#ff5050"), 3.0),
        QBrush(QColor("#c82828") ^ 0.7));
    sellZone->setZValue(1);
    auto *sellLabel = m_battleScene->addSimpleText(QStringLiteral("出售"));
    sellLabel->setBrush(Qt::white);
    QFont sellFont("Microsoft YaHei", 12);
    sellFont.setBold(true);
    sellLabel->setFont(sellFont);
    sellLabel->setPos(sellX + sellSize / 2.0 - sellLabel->boundingRect().width() / 2.0,
                      sellY + sellSize / 2.0 - sellLabel->boundingRect().height() / 2.0);
    sellLabel->setZValue(2);
}

// ============================================================================
// 战场刷新（只负责敌人绘制，我方单位由 refreshAllUnits 管理）
// ============================================================================
void MainWindow::refreshBattleGround()
{
    if (!m_battleScene || !m_gameManager)
        return;

    // 移除旧的敌人图形项（通过 data(0) tag 识别）
    QList<QGraphicsItem *> allItems = m_battleScene->items();
    for (auto *item : allItems)
    {
        if (item->data(0).toInt() == -1) // tag == -1 表示敌人临时项
        {
            m_battleScene->removeItem(item);
            delete item;
        }
    }

    const auto &enemies = m_gameManager->getEnemies();
    for (size_t i = 0; i < enemies.size(); ++i)
    {
        const EnemyInstance &enemy = enemies[i];
        if (!enemy.isAlive)
            continue;

        const double x = 1180.0 + (static_cast<double>(i % 3) * 140.0);
        const double y = 160.0 + (static_cast<double>(i / 3) * 180.0);
        const double radius = 34.0;
        const QColor color(240, 70, 70);

        QRectF bodyRect(x - radius, y - radius, radius * 2.0, radius * 2.0);
        auto *body = m_battleScene->addEllipse(bodyRect, QPen(Qt::black, 2), QBrush(color));
        body->setData(0, -1);
        body->setZValue(10);

        const double barWidth = radius * 3.0;
        const double barHeight = 8.0;
        const double barX = x - barWidth / 2.0;
        const double barY = y - radius - 24.0;
        auto *barBack = m_battleScene->addRect(QRectF(barX, barY, barWidth, barHeight),
                                               QPen(Qt::NoPen), QBrush(QColor(40, 40, 40, 220)));
        barBack->setData(0, -1);
        barBack->setZValue(20);

        const int maxHp = enemy.hp.getFinal();
        const double hpRatio = maxHp > 0
                                   ? std::clamp(static_cast<double>(enemy.currentHp) / static_cast<double>(maxHp), 0.0, 1.0)
                                   : 0.0;
        auto *barFront = m_battleScene->addRect(QRectF(barX, barY, barWidth * hpRatio, barHeight),
                                                QPen(Qt::NoPen), QBrush(QColor(220, 60, 60)));
        barFront->setData(0, -1);
        barFront->setZValue(21);

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
}

// ============================================================================
// 同步所有我方单位的图形项
// ============================================================================
void MainWindow::refreshAllUnits()
{
    if (!m_battleScene || !m_gameManager)
        return;

    const auto &units = m_gameManager->getPlayerAssets().ownedChesses;
    QSet<int> currentUuids;

    const bool isPrepare = (m_gameManager->getCurrentPhase() == RoundPhase::Prepare);

    for (const auto &unit : units)
    {
        currentUuids.insert(unit.uuid);
        UnitGraphicsItem *item = m_unitItems.value(unit.uuid, nullptr);

        if (!item)
        {
            item = new UnitGraphicsItem(unit.uuid);
            m_battleScene->addItem(item);
            m_unitItems[unit.uuid] = item;
            connect(item, &UnitGraphicsItem::dragFinished,
                    this, &MainWindow::onUnitDragFinished);
        }

        QPointF targetPos;
        double radius;
        QColor color;

        if (unit.deployed)
        {
            targetPos = QPointF(unit.posX, unit.posY);
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
        item->updateVisual(unit.name, unit.currentHp, unit.hp.getFinal(), color, radius, unit.starLevel);
    }

    // 清理已不存在的单位图形项
    QList<int> toRemove;
    for (auto it = m_unitItems.begin(); it != m_unitItems.end(); ++it)
    {
        if (!currentUuids.contains(it.key()))
            toRemove.append(it.key());
    }
    for (int uuid : toRemove)
    {
        delete m_unitItems.take(uuid);
    }
}

/// @brief 判定坐标是否在部署区
/// @param scenePos
/// @return
bool MainWindow::isInBattlefieldLegalZone(QPointF scenePos) const
{
    const double x = scenePos.x();
    const double y = scenePos.y();
    const double deployRight = kBattlefieldWidth / 2.0 - kBattlefieldMargin + kDeployRightExtension;
    return x >= kBattlefieldMargin &&
           x <= deployRight &&
           y >= kBattlefieldMargin &&
           y <= kBattlefieldHeight - kBattlefieldMargin;
}

/// @brief 判定坐标是否在备战席
/// @param scenePos
/// @return
bool MainWindow::isInBenchZone(QPointF scenePos) const
{
    const double y = scenePos.y();
    return y > static_cast<double>(kBattlefieldHeight) &&
           y < static_cast<double>(kBattlefieldHeight + kBenchAreaHeight);
}

/// @brief 判定坐标是否在出售区
/// @param scenePos
/// @return
bool MainWindow::isInSellZone(QPointF scenePos) const
{
    const double sellX = kBenchSlotStartX + PlayerAssets::maxBench * kBenchSlotWidth + 20.0;
    const double sellY = kBenchSlotStartY + (kBenchSlotHeight - 100.0) / 2.0;
    const double sellSize = 100.0;
    return scenePos.x() >= sellX && scenePos.x() <= sellX + sellSize &&
           scenePos.y() >= sellY && scenePos.y() <= sellY + sellSize;
}

/// @brief 找到最近的备战席槽位
/// @param scenePos
/// @return
int MainWindow::nearestBenchSlot(QPointF scenePos) const
{
    if (!m_gameManager)
        return -1;
    const auto &units = m_gameManager->getPlayerAssets().ownedChesses;

    // 标记已占用的槽位
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
            continue; // 跳过已占用槽位
        const double slotCx = kBenchSlotStartX + i * kBenchSlotWidth + (kBenchSlotWidth - 8) / 2.0;
        const double dist = std::abs(x - slotCx);
        if (dist < bestDist)
        {
            bestDist = dist;
            bestSlot = i;
        }
    }
    return bestSlot; // -1 表示备战席已满
}

/// @brief 拖拽结束处理
/// @param uuid 单位uuid
/// @param scenePos 结束位置
void MainWindow::onUnitDragFinished(int uuid, QPointF scenePos)
{
    if (!m_gameManager)
        return;

    if (m_gameManager->getCurrentPhase() != RoundPhase::Prepare)
    {
        refreshAllUnits();
        return;
    }

    auto &assets = m_gameManager->getPlayerAssets();
    auto &units = assets.ownedChesses;

    ChessInstance *unit = nullptr;
    for (auto &u : units)
    {
        if (u.uuid == uuid)
        {
            unit = &u;
            break;
        }
    }
    if (!unit)
    {
        refreshAllUnits();
        return;
    }

    const bool wasDeployed = unit->deployed;
    const int oldBenchSlot = unit->benchSlot;

    // ---- S: 出售（拖到红色方块） ----
    if (isInSellZone(scenePos))
    {
        m_gameManager->sellUnit(uuid);
        refreshAllUnits();
        refreshSceneLabels();
        return;
    }

    bool changed = false;

    // ---- A: 备战席 → 战场合法区域 ----
    if (!wasDeployed && isInBattlefieldLegalZone(scenePos))
    {
        if (!assets.battlefieldFull())
        {
            unit->deployed = true;
            unit->benchSlot = -1;
            unit->posX = scenePos.x();
            unit->posY = scenePos.y();
            changed = true;
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
        if (targetSlot >= 0) // 有空槽才撤回
        {
            unit->deployed = false;
            unit->benchSlot = targetSlot;
            unit->posX = 0;
            unit->posY = 0;
            changed = true;
            print(QString("Undeploy unit %1 to bench slot %2").arg(uuid).arg(targetSlot));
        }
    }
    // ---- C: 战场内移动 ----
    else if (wasDeployed && isInBattlefieldLegalZone(scenePos))
    {
        unit->posX = scenePos.x();
        unit->posY = scenePos.y();
        changed = true;
        print(QString("Move unit %1 on battlefield to (%2,%3)")
                  .arg(uuid)
                  .arg(scenePos.x())
                  .arg(scenePos.y()));
    }
    // ---- D: 备战席内交换/移动 ----
    else if (!wasDeployed && isInBenchZone(scenePos))
    {
        // 直接找最近的槽位（含已占用的，用于交换）
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
            changed = true;
        }
    }
    // ---- E: 非法区域 → 弹回（不做数据修改） ----

    refreshAllUnits();
}

// ============================================================================
// 标签刷新
// ============================================================================
void MainWindow::refreshSceneLabels()
{
    if (!m_gameManager)
        return;

    if (auto *roundCount = findChild<QLabel *>("roundCount"))
        roundCount->setText(QString("第%1门").arg(m_gameManager->getRoundNumber()));
    if (auto *totalCredit = findChild<QLabel *>("totalCredit"))
        totalCredit->setText(QString("金币：%1").arg(m_gameManager->getPlayerAssets().gold));
    if (auto *roundValue = findChild<QLabel *>("roundValue"))
    {
        const auto &assets = m_gameManager->getPlayerAssets();
        roundValue->setText(QString("战场：%1/%2  备战：%3/%4")
                                .arg(assets.deployedCount())
                                .arg(PlayerAssets::maxBattlefield)
                                .arg(assets.benchCount())
                                .arg(PlayerAssets::maxBench));
    }
    if (auto *gpa = findChild<QLabel *>("gpa"))
        gpa->setText(QString("塔血：%1").arg(m_gameManager->getTowerHp()));
}

// ============================================================================
// 商店
// ============================================================================

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

/// @brief 该函数专门用来显示跳字（如伤害数字、回血数字等）
/// @param text 要显示的文本内容
/// @param x 场景坐标系中的 x 坐标（中心点）
/// @param y 场景坐标系中的 y 坐标（底部）
/// @param color 文本颜色，期望为 CSS 颜色字符串（如 "#FF0000" 或 "rgb(255,0,0)"）
void MainWindow::showSplashText(const QString &text, double x, double y, const QString &color)
{
    if (!m_battleScene)
        return;

    QFont ftFont("Microsoft YaHei", 20);
    ftFont.setBold(true);

    auto *font = m_battleScene->addSimpleText(text);
    font->setFont(ftFont);
    font->setBrush(QColor(color));
    font->setPos(x - font->boundingRect().width() / 2.0, y - 30.0); // 以文本底部中心为锚点
    font->setTransformOriginPoint(font->boundingRect().center());
    font->setZValue(500);
    font->setScale(4.0);

    auto *shadow = m_battleScene->addSimpleText(text);
    shadow->setFont(ftFont);
    shadow->setBrush(QColor(0, 0, 0, 160));
    shadow->setPos(font->pos() + QPointF(1, 1));
    shadow->setTransformOriginPoint(shadow->boundingRect().center());
    shadow->setZValue(499);
    shadow->setScale(4.0);

    auto *anim = new QVariantAnimation(this);
    anim->setDuration(300);
    anim->setStartValue(4.0);
    anim->setEndValue(1.0);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    connect(anim, &QVariantAnimation::valueChanged, // valueChanged 在每一帧更新时触发，传递当前的动画值给匿名函数参数
            this, [font, shadow](const QVariant &val)
            {
                double s = val.toDouble();
                font->setScale(s);
                shadow->setScale(s); });

    connect(anim, &QVariantAnimation::finished, anim, &QObject::deleteLater);
    anim->start(QAbstractAnimation::DeleteWhenStopped);

    QTimer::singleShot(800, this, [font, shadow]()
                       {
        if (font->scene()) font->scene()->removeItem(font);
        if (shadow->scene()) shadow->scene()->removeItem(shadow);
        delete font;
        delete shadow; });
}

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
                    "本回合学分绩：%6 / 4.0")
                    .arg(victory ? QStringLiteral("✨ 胜利！") : QStringLiteral("💀 失败..."))
                    .arg(goldEarned)
                    .arg(expEarned)
                    .arg(towerHp)
                    .arg(m_gameManager->getMaxTowerHp())
                    .arg(gpa, 0, 'f', 2));
    box.setStandardButtons(QMessageBox::Ok);
    box.exec();

    QTimer::singleShot(300, this, [this]()
                       {
        if (m_gameManager)
            m_gameManager->nextRound(); });
}

// ============================================================================
// 游戏初始化（首次点击开始游戏时触发）
// ============================================================================
void MainWindow::initGame()
{
    print("Starting game");
    switchScene(Scene::MainGame);

    if (!m_gameManager)
    {
        m_gameManager = new GameManager(this);
        if (!m_database)
            m_database = new DatabaseManager(
                QCoreApplication::applicationDirPath() + "/assets/entities", this);
        m_gameManager->setDatabase(m_database);
        connect(m_gameManager, &GameManager::floatingText, this, &MainWindow::showSplashText);
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
        connect(m_gameManager, &GameManager::roundEnded, this, &MainWindow::showRoundResult);

        connect(ui->startRoundButton, &QPushButton::clicked, this, [this]()
                {
            if (!m_gameManager) return;
            m_gameManager->startRound(m_gameManager->getRoundNumber());
            refreshBattleGround();
            refreshAllUnits(); });

        connect(ui->openShopButton, &QPushButton::clicked, this, &MainWindow::onShopOpenClicked);
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
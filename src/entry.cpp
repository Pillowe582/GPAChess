#include "mainwindow.h"
#include "print.h"
#include "ui_entry.h"
#include "logo_player.h"
#include "game_manager.h"
#include "unit_graphics_item.h"
#include "database_manager.h"
#include "shop_window.h"
#include "renderer.h"

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
/// 本文件专门处理UI初始化与UI逻辑交互

// ============================================================================
// % 程序生命周期
// ============================================================================

/// @brief 负责启动ui
/// @param parent
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    print("进入主窗口入口");
    ui->setupUi(this);
    initUi();

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
    print("开始初始化UI");
    ui->sceneController->setCurrentIndex(Scene::EntryLogo);
    this->setFixedSize(1920, 1080);
    this->setWindowTitle("GPAutoChess");
    setupGameScene();
    print("UI初始化完毕");
}

/// @brief 首次点击"开始游戏"触发 —— 创建核心对象并绑定所有信号槽
void MainWindow::initGame()
{
    print("启动！！");
    switchScene(Scene::MainGame);
    if (m_gameManager)
    {
        delete m_gameManager;
    }
    if (m_renderer)
    {
        delete m_renderer;
    }
    // 刷新UI状态
    ui->startRoundButton->setEnabled(1);
    ui->openShopButton->setEnabled(1);

    // 创建游戏主控
    m_gameManager = new GameManager(this);
    m_gameManager->initialize();

    // 创建渲染器并注入
    m_renderer = new Renderer(m_battleScene, this);
    m_gameManager->setRenderer(m_renderer);

    // 回合阶段改变 → UI 刷新
    connect(m_gameManager, &GameManager::phaseChanged, this, [this](RoundPhase p)
            {
            print(QString("回合阶段改变至: %1").arg(static_cast<int>(p)));
            ui->startRoundButton->setEnabled(p == RoundPhase::Prepare);
            ui->openShopButton->setEnabled(p == RoundPhase::Prepare);
            refreshScene(RefreshAll); });

    // tick 信号 → 刷新单位
    connect(m_gameManager, &GameManager::tick, this, [this]()
            { refreshScene(RefreshUnits); });

    // 回合结束 → 显示结果
    connect(m_gameManager, &GameManager::roundEnded,
            this, &MainWindow::showRoundResult);

    // 本局游戏结束 → 显示最终成绩
    connect(m_gameManager, &GameManager::gameOver,
            this, &MainWindow::showGameOver);

    // 点击回合开始按钮 → 通知 GameManager 开始回合
    connect(ui->startRoundButton, &QPushButton::clicked, this, [this]()
            {
            if (!m_gameManager) return;
            m_gameManager->startRound(m_gameManager->getRoundNumber());
            refreshScene(RefreshAll); });

    // 点击商店按钮 → 打开商店窗口
    connect(ui->openShopButton, &QPushButton::clicked,
            this, &MainWindow::onShopOpenClicked);

    refreshScene(RefreshAll);
}

MainWindow::~MainWindow()
{
    print("Mainba out");
    delete ui;
}

// ============================================================================
// % UI切换
// ==========================================================================

/// @brief 切换场景（QStackedWidget 跳转）
/// @param scene 目标场景
/// @return 返回切换前的场景索引
int MainWindow::switchScene(Scene scene)
{
    print("Switching to scene " + QString::number(scene));
    int currentScene = ui->sceneController->currentIndex();
    ui->sceneController->setCurrentIndex(scene);
    return currentScene;
}

/// @brief 用于输出完整资源路径
/// @param relativePath 输入相对路径（相对于 assets/）
/// @return 返回绝对路径，如果资源不存在则返回 assets/ 相对路径
QString MainWindow::assetPath(const QString &relativePath) const
{
    const QString base = QCoreApplication::applicationDirPath();
    const QString candidate = QDir(base).filePath(QString("assets/%1").arg(relativePath));
    if (QFileInfo::exists(candidate))
        return candidate;
    print("未找到资源 " + candidate);
    return QString("assets/%1").arg(relativePath);
}

/// @brief 打开商店窗口（仅在准备阶段可用）
void MainWindow::onShopOpenClicked()
{
    m_gameManager->openShop(this, &MainWindow::refreshScene); // 商店关闭后的回调：刷新整个场景
}

// =========================================================================
// % 战场绘制
// ============================================================================

/// @brief 创建 QGraphicsScene 并绘制所有静态元素（背景、塔、备战席、出售区之类的）
void MainWindow::setupGameScene()
{
    QGraphicsView *battleView = ui->mainBattleGround;
    if (!battleView)
    {
        print("Error: 战场画布是滚木");
        return;
    }

    // 给QGraphicsView设置QGraphicsScene
    m_battleScene = new QGraphicsScene(this);
    battleView->setScene(m_battleScene);
    battleView->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    battleView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    battleView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    const int totalHeight = kBattlefieldHeight + kBenchAreaHeight;
    m_battleScene->setSceneRect(0, 0, kBattlefieldWidth, totalHeight);

    // 战场背景贴图
    const QString bgPath = assetPath("texture/battleground/sand.png");
    QPixmap bg(bgPath);
    if (!bg.isNull())
    {
        QSize sceneSize(kBattlefieldWidth, kBattlefieldHeight);
        const int safeMargin = 100;
        QSize targetSize(sceneSize.width() + safeMargin * 2,
                         sceneSize.height() + safeMargin * 2); // 稍微放得比战场大些
        auto scaled = bg.scaled(targetSize, Qt::KeepAspectRatioByExpanding,
                                Qt::SmoothTransformation);
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

    // 部署区提示边框
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

    // ============================================
    // % 备战席绘制
    // ================================================

    // 备战席区域背景
    const int horizontalMargin = 50;
    const double benchY = static_cast<double>(kBattlefieldHeight);
    auto *benchBg = m_battleScene->addRect(
        QRectF(horizontalMargin, benchY, kBattlefieldWidth - 2 * horizontalMargin, kBenchAreaHeight),
        QPen(Qt::NoPen), QBrush(QColor("#686868")));
    benchBg->setZValue(-10);

    // 备战席槽位
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
    m_sellLabel = m_battleScene->addSimpleText(QStringLiteral("出售"));
    m_sellLabel->setBrush(Qt::white);
    QFont sellFont("Microsoft YaHei", 12);
    sellFont.setBold(true);
    m_sellLabel->setFont(sellFont);
    m_sellLabel->setPos(sellX + sellSize / 2.0 - m_sellLabel->boundingRect().width() / 2.0,
                        sellY + sellSize / 2.0 - m_sellLabel->boundingRect().height() / 2.0);
    m_sellLabel->setZValue(2);
}

// ===========================================================================
// % 刷新
// ============================================================================
void MainWindow::refreshScene(int flags)
{
    if (flags & RefreshUnits)
    {
        refreshAllUnits();
    }
    if (flags & RefreshLabels)
    {
        refreshSceneLabels();
    }
}

/// @brief 刷新我方单位（部署 → 用 posX/posY；备战 → 按槽位算坐标）
void MainWindow::refreshAllUnits()
{
    if (!m_battleScene || !m_gameManager)
        return;

    const auto &allyChesses = m_gameManager->getPlayerAssets().ownedChesses;
    QSet<int> currentUuids;
    const bool isPreparePhase = (m_gameManager->getCurrentPhase() == RoundPhase::Prepare);

    for (const auto &chess : allyChesses)
    {
        currentUuids.insert(chess->getUuid());                                 // 记录当前存在的单位 UUID
        UnitGraphicsItem *item = m_unitItems.value(chess->getUuid(), nullptr); // 查找已有的图形项

        if (!item)
        {
            // 如果uuid对应不到图形项，则创建新的图形项并加入场景
            item = new UnitGraphicsItem(chess->getUuid(), nullptr,
                                        m_gameManager, m_renderer);
            m_battleScene->addItem(item);
            m_unitItems[chess->getUuid()] = item;
            connect(item, &UnitGraphicsItem::dragStarted,
                    this, &MainWindow::onUnitDragStarted);
            connect(item, &UnitGraphicsItem::dragFinished,
                    this, &MainWindow::onUnitDragFinished);
        }

        // 计算我方棋子目标位置
        QPointF targetPos;

        if (chess->deployed)
        {
            targetPos = QPointF(chess->transform.x, chess->transform.y);
        }
        else
        {
            int slot = chess->benchSlot;
            if (slot < 0 || slot >= PlayerAssets::maxBench)
                slot = 0;
            const double cx = kBenchSlotStartX + slot * kBenchSlotWidth + (kBenchSlotWidth - 8) / 2.0;
            const double cy = kBenchSlotStartY + kBenchSlotHeight / 2.0;
            targetPos = QPointF(cx, cy);
        }

        item->setVisible(chess->isAlive);
        item->setPos(targetPos);
        item->setDraggable(isPreparePhase && !chess->isTower);
        item->update(); // 强制触发重绘，确保图片立即更新

        // 视觉由 behavior 决定，entry 只提供坐标和实例引用
        // 只有存活的单位才渲染
        if (chess->isAlive && chess->behavior && m_gameManager->getCurrentPhase() != RoundPhase::Prepare)
            chess->behavior->renderSelf(*chess, *m_renderer, targetPos.x(), targetPos.y());
    }

    // 清理已不存在的单位图形项
    QList<int> toRemove;
    for (auto it = m_unitItems.begin(); it != m_unitItems.end(); ++it)
    {
        if (!currentUuids.contains(it.key()))
            toRemove.append(it.key());
    }
    for (int uuid : toRemove)
        delete m_unitItems.take(uuid);

    // 渲染 behavior 排队的绘制指令
    // 准备阶段：需要 beginFrame 清旧帧 + flush 渲染
    // 对战阶段：beginFrame 已由 executeAttackCycle 调用，只需 flush
    if (isPreparePhase)
        m_renderer->beginFrame();
    m_renderer->flush();
}

/// @brief 根据当前 GameManager 状态更新所有 UI 标签
void MainWindow::refreshSceneLabels()
{
    if (!m_gameManager)
        return;

    int round = m_gameManager->getRoundNumber();
    int credit = m_gameManager->getRoundInfos()[round].creditWorth;
    const auto &assets = m_gameManager->getPlayerAssets();

    if (auto *lbl = findChild<QLabel *>("roundCount"))
        lbl->setText(QString("第%1门：XXX, %2cr").arg(round).arg(credit));
    if (auto *lbl = findChild<QLabel *>("totalCredit"))
        lbl->setText(QString("已修学分：%1").arg(m_gameManager->getPreviousCredits()));
    if (auto *lbl = findChild<QLabel *>("gpa"))
        lbl->setText(QString("GPA：%1").arg(m_gameManager->getAverageGpa(), 0, 'f', 2));
    if (auto *lbl = findChild<QLabel *>("onBattleGroundChessCount"))
        lbl->setText(QString("上场：%1/%2").arg(assets.deployedCount()).arg(PlayerAssets::maxBattlefield));
    if (auto *lbl = findChild<QLabel *>("goldCount"))
        lbl->setText(QString("金币：%1").arg(assets.gold));
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
/// @param uuid 拖拽单位的 UUID
/// @param scenePos 拖拽起始点
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

/// @brief 判断坐标是否在部署区
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
        if (!u->deployed && u->benchSlot >= 0 && u->benchSlot < PlayerAssets::maxBench)
            occupied[u->benchSlot] = true;

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
        refreshScene(RefreshAll); // 弹回并刷新标签
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
        refreshScene(RefreshAll); // 弹回并刷新标签
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
        refreshScene(RefreshAll);
        return;
    }

    // ---- A: 备战席 → 战场 ----
    if (!wasDeployed && isInBattlefieldLegalZone(scenePos))
    {
        if (!assets.battlefieldFull())
        {
            unit->deployed = true;
            unit->benchSlot = -1;
            unit->transform.x = scenePos.x();
            unit->transform.y = scenePos.y();
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
            unit->transform.x = 0;
            unit->transform.y = 0;
            print(QString("Undeploy unit %1 to bench slot %2").arg(uuid).arg(targetSlot));
        }
    }
    // ---- C: 战场内移动 ----
    else if (wasDeployed && isInBattlefieldLegalZone(scenePos))
    {
        unit->transform.x = scenePos.x();
        unit->transform.y = scenePos.y();
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
                if (!u->deployed && u->benchSlot == rawSlot && u->getUuid() != uuid)
                {
                    occupant = u.get();
                    break;
                }
            }
            if (occupant)
            {
                occupant->benchSlot = oldBenchSlot;
                unit->benchSlot = rawSlot;
                print(QString("Swap bench slots: %1 ↔ %2").arg(uuid).arg(occupant->getUuid()));
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
    refreshScene(RefreshAll); // 刷新单位位置和标签（上场数、金币等）
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
    refreshScene(RefreshAll);

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
                    "本回合学分绩：%6 * %7cr")
                    .arg(victory ? QStringLiteral("✨ 胜利！") : QStringLiteral("💀 失败..."))
                    .arg(goldEarned)
                    .arg(expEarned)
                    .arg(towerHp)
                    .arg(m_gameManager->getMaxTowerHp())
                    .arg(gpa, 0, 'f', 2)
                    .arg(m_gameManager->getRoundInfos()[m_gameManager->getRoundNumber()].creditWorth));
    box.setStandardButtons(QMessageBox::Ok);
    box.exec();

    if (m_gameManager)
        m_gameManager->nextRound();
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

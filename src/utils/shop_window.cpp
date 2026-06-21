#include "shop_window.h"
#include "ui_shop_window.h"
#include "database_manager.h"
#include "game_manager.h"
#include "entity/ally_behavior.h"
#include "print.h"

#include <QRandomGenerator>
#include <QMessageBox>
#include <QCloseEvent>

ShopWindow::ShopWindow(DatabaseManager *db, GameManager *gm, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::shopWindow), m_db(db), m_gameManager(gm)
{
    ui->setupUi(this);
    print("打开商店窗口…");
    setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
    setWindowModality(Qt::ApplicationModal);

    m_listModel = new QStringListModel(this);
    ui->shopChessList->setModel(m_listModel);
    ui->shopChessList->setEditTriggers(QAbstractItemView::NoEditTriggers);

    connect(ui->shopChessList, &QListView::clicked, this, &ShopWindow::onShopChessItemClicked);
    connect(ui->purchaseButton, &QPushButton::clicked, this, &ShopWindow::onPurchaseClicked);
    connect(ui->shopRefreshButton, &QPushButton::clicked, this, &ShopWindow::onRefreshClicked);
    updateGoldLabel();
    refreshShop();
}

ShopWindow::~ShopWindow() { delete ui; }

// % 刷新商店

void ShopWindow::onRefreshClicked()
{
    if (m_gameManager->getPlayerAssets().gold < 2)
        return;
    m_gameManager->getPlayerAssets().gold -= 2;
    refreshShop();
}

/// @brief 刷新商店物品列表
void ShopWindow::refreshShop()
{
    m_currentShopItems.clear();
    m_selectedIndex = -1;
    ui->descriptionLabel->setText("");
    ui->propertyLabel->setText("");
    ui->purchaseButton->setEnabled(false);

    const auto &all = m_db->allAllyConfigs();
    if (all.empty())
        return;

    auto rng = QRandomGenerator::global();
    QStringList names;
    for (int i = 0; i < 5 && !all.empty(); ++i)
    {
        int idx = rng->bounded(static_cast<int>(all.size()));
        m_currentShopItems.push_back(all[idx]);
        names << QString("%1  ￥%2").arg(all[idx].name).arg(all[idx].cost);
    }
    m_listModel->setStringList(names);
    updateGoldLabel();
}

//  % 展示角色信息
void ShopWindow::onShopChessItemClicked(const QModelIndex &index)
{
    int row = index.row();
    if (row < 0 || row >= static_cast<int>(m_currentShopItems.size()))
        return;
    m_selectedIndex = row;
    const auto &cfg = m_currentShopItems[row];
    ui->descriptionLabel->setText(cfg.description);
    ui->propertyLabel->setText(
        QString("HP:%1  ATK:%2  DEF:%3\n攻速:%4  范围:%5\n羁绊:%6")
            .arg(cfg.baseHp)
            .arg(cfg.baseAtk)
            .arg(cfg.baseDef)
            .arg(cfg.baseAttackSpeed)
            .arg(cfg.attackRange)
            .arg(cfg.bonds.join(", ")));
    ui->purchaseButton->setEnabled(true);
}

// % 购买角色
void ShopWindow::onPurchaseClicked()
{
    if (m_selectedIndex < 0 || m_selectedIndex >= static_cast<int>(m_currentShopItems.size()))
        return;

    const AllyConfig &cfg = m_currentShopItems[m_selectedIndex];
    auto &assets = m_gameManager->getPlayerAssets();

    // 检查金币和备战席是否足够
    if (assets.gold < cfg.cost)
    {
        QMessageBox::warning(this, QStringLiteral("金币不足"), QStringLiteral("你没有足够的金币购买此角色！"));
        return;
    }
    if (assets.benchFull())
    {
        QMessageBox::warning(this, QStringLiteral("备战席已满"), QStringLiteral("备战席已满，无法购买更多角色！"));
        return;
    }

    // 扣除金币，创建棋子实例并放入备战席
    assets.gold -= cfg.cost;
    int slot = assets.firstEmptyBenchSlot();
    auto inst = std::make_unique<ChessInstance>(cfg, m_gameManager);

    inst->deployed = false;
    inst->benchSlot = slot;
    inst->behavior.reset(createAllyBehavior(inst->behaviorId));
    assets.ownedChesses.push_back(std::move(inst));

    print(QString("花费 %1 金币购买 %2，位置 %3，目前共有 %4 个角色").arg(cfg.cost).arg(cfg.name).arg(slot).arg(assets.ownedChesses.size()));

    // 检查是否可以升星合并
    m_gameManager->checkAndMergeStars();

    // 更新商店列表数据
    m_currentShopItems.erase(m_currentShopItems.begin() + m_selectedIndex);
    m_selectedIndex = -1;

    // 刷新商店UI显示
    ui->descriptionLabel->setText("");
    ui->propertyLabel->setText("");
    ui->purchaseButton->setEnabled(false);
    QStringList names;
    for (const auto &item : m_currentShopItems)
        names << QString("%1  ￥%2").arg(item.name).arg(item.cost);
    m_listModel->setStringList(names);
    updateGoldLabel();
}

void ShopWindow::updateGoldLabel()
{
    int gold = m_gameManager->getPlayerAssets().gold;
    ui->shopGoldLabel->setText(QString("金币：%1").arg(gold));
    ui->shopRefreshButton->setEnabled(gold >= 2);
}

// % 窗口事件
void ShopWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    updateGoldLabel();
}

void ShopWindow::closeEvent(QCloseEvent *event)
{
    print("关闭商店窗口");
    emit shopClosed();
    QMainWindow::closeEvent(event);
}

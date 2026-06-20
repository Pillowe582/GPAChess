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
    setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
    setWindowModality(Qt::ApplicationModal);

    m_listModel = new QStringListModel(this);
    ui->shopChessList->setModel(m_listModel);
    ui->shopChessList->setEditTriggers(QAbstractItemView::NoEditTriggers);

    connect(ui->shopChessList, &QListView::clicked, this, &ShopWindow::onShopItemClicked);
    connect(ui->purchaseButton, &QPushButton::clicked, this, &ShopWindow::onPurchaseClicked);
    connect(ui->shopRefreshButton, &QPushButton::clicked, this, &ShopWindow::onRefreshClicked);

    refreshShop();
}

ShopWindow::~ShopWindow() { delete ui; }

void ShopWindow::refreshShop()
{
    m_currentShopItems.clear();
    m_selectedIndex = -1;
    ui->descriptionLabel->setText("");
    ui->propertyLabel->setText("");
    ui->purchaseButton->setEnabled(false);

    const auto &all = m_db->allChessConfigs();
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

void ShopWindow::onShopItemClicked(const QModelIndex &index)
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

void ShopWindow::onPurchaseClicked()
{
    if (m_selectedIndex < 0 || m_selectedIndex >= static_cast<int>(m_currentShopItems.size()))
        return;

    const ChessConfig &cfg = m_currentShopItems[m_selectedIndex];
    auto &assets = m_gameManager->getPlayerAssets();

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

    assets.gold -= cfg.cost;
    int slot = assets.firstEmptyBenchSlot();
    ChessInstance inst(cfg);
    inst.deployed = false;
    inst.benchSlot = slot;
    inst.behavior.reset(createAllyBehavior(inst.behaviorId));
    assets.ownedChesses.push_back(std::move(inst));

    print(QString("Purchased %1 for %2 gold, slot %3").arg(cfg.name).arg(cfg.cost).arg(slot));

    m_gameManager->checkAndMergeStars();

    m_currentShopItems.erase(m_currentShopItems.begin() + m_selectedIndex);
    m_selectedIndex = -1;
    ui->descriptionLabel->setText("");
    ui->propertyLabel->setText("");
    ui->purchaseButton->setEnabled(false);

    QStringList names;
    for (const auto &item : m_currentShopItems)
        names << QString("%1  ￥%2").arg(item.name).arg(item.cost);
    m_listModel->setStringList(names);
    updateGoldLabel();
}

void ShopWindow::onRefreshClicked()
{
    const int cost = 2;
    auto &assets = m_gameManager->getPlayerAssets();
    if (assets.gold < cost)
    {
        QMessageBox::warning(this, QStringLiteral("金币不足"), QStringLiteral("刷新需要4金币！"));
        return;
    }
    assets.gold -= cost;
    refreshShop();
}

void ShopWindow::updateGoldLabel()
{
    ui->shopGoldLabel->setText(QString("金币：%1").arg(m_gameManager->getPlayerAssets().gold));
}

void ShopWindow::closeEvent(QCloseEvent *event)
{
    emit shopClosed();
    QMainWindow::closeEvent(event);
}

#ifndef SHOPWINDOW_H
#define SHOPWINDOW_H

#include <QMainWindow>
#include <QStringListModel>
#include <vector>
#include "state.h"

namespace Ui
{
    class shopWindow;
}
class DatabaseManager;
class GameManager;

class ShopWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit ShopWindow(DatabaseManager *db, GameManager *gm, QWidget *parent = nullptr);
    ~ShopWindow();
    void refreshShop();

protected:
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;

signals:
    void shopClosed();

private slots:
    void onShopItemClicked(const QModelIndex &index);
    void onPurchaseClicked();
    void onRefreshClicked();

private:
    void updateGoldLabel();
    Ui::shopWindow *ui;
    DatabaseManager *m_db;
    GameManager *m_gameManager;
    QStringListModel *m_listModel;
    std::vector<ChessConfig> m_currentShopItems;
    int m_selectedIndex = -1;
};

#endif

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QColor>
#include <QString>
#include <QMap>
#include <QPointF>

class QGraphicsScene;
class GameManager;
class UnitGraphicsItem;
class ShopWindow;
class DatabaseManager;

QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    enum Scene : int
    {
        EntryLogo = 0,
        EntryMenu = 1,
        MainGame = 2,
    };
    int switchScene(Scene scene);

private slots:
    /// 单位拖拽结束后的处理
    void onUnitDragFinished(int uuid, QPointF scenePos);
    /// 打开商店
    void onShopOpenClicked();

private:
    Ui::MainWindow *ui;
    void initUi();
    void initGame();
    void setupGameScene();
    void refreshBattleGround();
    void refreshAllUnits();
    void refreshSceneLabels();
    void showSplashText(const QString &text, double x, double y, const QString &color);
    void showRoundResult(bool victory);
    QString assetPath(const QString &relativePath) const;

    // 合法性判定
    bool isInBattlefieldLegalZone(QPointF scenePos) const;
    bool isInBenchZone(QPointF scenePos) const;
    bool isInSellZone(QPointF scenePos) const;
    int nearestBenchSlot(QPointF scenePos) const;

    // 常量
    static constexpr int kBattlefieldWidth = 1920;
    static constexpr int kBattlefieldHeight = 800;    // 战场可玩区域高度
    static constexpr int kBenchAreaHeight = 180;      // 底部备战栏高度
    static constexpr int kBattlefieldMargin = 50;     // 战场部署区边界留白
    static constexpr int kDeployRightExtension = 200; // 部署区右侧额外扩展
    static constexpr int kBenchSlotWidth = 158;
    static constexpr int kBenchSlotHeight = 100;
    static constexpr double kBenchSlotStartX = 20.0;
    static constexpr double kBenchSlotStartY = kBattlefieldHeight + 50.0;

    // 管理
    GameManager *m_gameManager = nullptr;
    QGraphicsScene *m_battleScene = nullptr;
    QMap<int, UnitGraphicsItem *> m_unitItems;
    ShopWindow *m_shopWindow = nullptr;
    DatabaseManager *m_database = nullptr;
};

#endif
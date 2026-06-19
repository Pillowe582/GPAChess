#include "mainwindow.h"
#include "print.h"
#include "ui_entry.h" // Qt自动生成的，指向entry.ui
#include "logo_player.h"
MainWindow::MainWindow(QWidget *parent) // 让主窗口构造的魔法
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    initUi();
    print("Entry point initialized");
    LogoPlayer *logoPlayer = new LogoPlayer(ui->logoDisplayLabel, this); // 播放开屏动画
    connect(logoPlayer, &LogoPlayer::logoPlayFinished, this, [this]()
            {
                this->switchScene(Scene::EntryMenu); // 播完切到主菜单
            });
}

MainWindow::~MainWindow() // 让主窗口析构的魔法
{
    print("Mainba out");
    delete ui;
}

int MainWindow::switchScene(Scene scene) // 让场景切换的魔法
{
    print("Switching to scene " + QString::number(scene));
    int currentScene = ui->sceneController->currentIndex();
    ui->sceneController->setCurrentIndex(scene);
    return currentScene; // 返回切换之前的scene
}

void MainWindow::initUi() // 让UI初始化的魔法
{
    ui->sceneController->setCurrentIndex(Scene::EntryLogo);
    this->setFixedSize(1920, 1080);
}
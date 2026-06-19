#include "mainwindow.h"
#include "print.h"
#include "ui_entry.h"
#include "logo_player.h"
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    initUi();
    print("Entry point initialized");
    LogoPlayer *logoPlayer = new LogoPlayer(ui->logoDisplayLabel, this); // 播放开屏动画
    connect(logoPlayer, &LogoPlayer::logoPlayFinished, this, [this]()
            {
                // 播完切到主菜单
                this->switchScene(Scene::EntryMenu); });
    connect(ui->startGameButton, &QPushButton::clicked, this, [this]()
            {
                // 点击开始游戏按钮切到主游戏界面
                print("Starting game");
                this->switchScene(Scene::MainGame); });

    connect(ui->settingsButton, &QPushButton::clicked, this, [this]()
            { 
                // 点击设置按钮切到设置界面
                print("Opening settings"); });
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
    this->setWindowTitle("GPAutoChess");
}

MainWindow::~MainWindow()
{
    print("Mainba out");
    delete ui;
}
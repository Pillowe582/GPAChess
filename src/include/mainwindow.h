#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

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

private:
    Ui::MainWindow *ui;
    void initUi();
};

#endif
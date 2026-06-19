#include "mainwindow.h"
#include "ui_entry.h" // Qt自动生成的，指向entry.ui
#include <QDebug>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    timer.start();
    ui->setupUi(this);
    qDebug() << timer.elapsed() << "Entry point initialized";
}

MainWindow::~MainWindow()
{
    qDebug() << timer.elapsed() << "Mainba out";
    delete ui;
}

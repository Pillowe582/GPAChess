#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/assets/MainIcon.ico"));

    MainWindow window;
    window.show();
    int result = app.exec();
    return result;
}
#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    qputenv("QT_ENABLE_HIGHDPI_SCALING", "0"); // 禁用高DPI缩放
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/assets/MainIcon.ico"));

    MainWindow window;
    window.show();
    int result = app.exec();
    return result;
}
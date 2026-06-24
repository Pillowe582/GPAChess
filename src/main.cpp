#include "mainwindow.h"
#include <QApplication>
#include <fstream>
#include <iostream>
#include <QDateTime>
#include <QString>

int main(int argc, char *argv[])
{
    qputenv("QT_ENABLE_HIGHDPI_SCALING", "0"); // 禁用高DPI缩放
    try
    {
        QApplication app(argc, argv);

        printf("App Path: %s\n", QCoreApplication::applicationDirPath().toStdString().c_str());
        app.setWindowIcon(QIcon(":/assets/icon.ico")); 

        MainWindow window;
        window.show();
        int result = app.exec();

        return result;
    }
    catch (const std::exception &e)
    {
        QString logPath = QCoreApplication::applicationDirPath() + "/crash_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".log";

        std::ofstream logFile(logPath.toStdString(), std::ios::app);
        logFile << "Error: " << e.what() << std::endl;
        std::cout << "Error: " << e.what() << std::endl;

        return -1;
    }
    catch (...)
    {
        QString logPath = QCoreApplication::applicationDirPath() + "/crash_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".log";

        std::ofstream logFile(logPath.toStdString(), std::ios::app);
        logFile << "Unknown error" << std::endl;
        std::cout << "Unknown error" << std::endl;

        return -1;
    }
}
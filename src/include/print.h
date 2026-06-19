#ifndef PRINT_H
#define PRINT_H
#include <QString>
#include <QDateTime>

// 这个文件提供了一个简单的print函数，用于在控制台输出带有时间戳的日志信息，方便调试和性能分析。
extern const QDateTime starttime;

void print(const QString &text);
#endif
#include "print.h"

#include <QDebug>

const QDateTime starttime = QDateTime::currentDateTimeUtc();

void print(const QString &text)
// 计算从程序启动到现在的时间差，并在日志中输出
{
    const qint64 elapsedMs = starttime.msecsTo(QDateTime::currentDateTimeUtc());
    qDebug().noquote() << QStringLiteral("[@ %1] %2").arg(elapsedMs).arg(text);
}
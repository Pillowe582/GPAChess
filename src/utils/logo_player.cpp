#include "logo_player.h"
#include "print.h"

LogoPlayer::LogoPlayer(QLabel *uiLabel, QWidget *parent) : QWidget(parent), currentFrame(0)
// 传入一个QLabel指针作为序列帧的显示区域
{
    print("LogoPlayer initialized");
    logoLabel = uiLabel;
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &LogoPlayer::updateFrame);
    timer->start(1000 / fps);
}

void LogoPlayer::updateFrame() // 让LogoPlayer更新帧的魔法
{
    if (currentFrame >= maxFrame)
    // 如果已经播放完所有帧，重置为第一帧，并发出播放完成的信号
    {
        currentFrame = maxFrame;
        emit logoPlayFinished();
        delete timer;
        return;
    }

    QString path = QString("assets/logo_frames/logo_frame_%1.png")
                       .arg(currentFrame, 3, 10, QChar('0')); // 构造资源路径，命名为logo_frame_000.png
    QPixmap pixmap(path);
    if (pixmap.isNull())
    {
        qDebug() << "Failed to load frame:" << path;
        delete timer;
        return;
    }
    logoLabel->setPixmap(pixmap);
    currentFrame++;
}
LogoPlayer::~LogoPlayer()
{
    timer->stop();
    delete timer;
}
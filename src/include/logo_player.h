#ifndef LOGO_PLAYER_H
#define LOGO_PLAYER_H

#include <QWidget>
#include <QLabel>
#include <QTimer>

class LogoPlayer : public QWidget
// 专门用来播放开屏动画
{
    Q_OBJECT

public:
    explicit LogoPlayer(QLabel *uiLabel, QWidget *parent = nullptr);
    ~LogoPlayer();

signals:
    void logoPlayFinished();

private slots:
    void updateFrame();

private:
    QLabel *logoLabel;
    QTimer *timer;
    int currentFrame;
    const int maxFrame = 404;
    const float fps = 60;
};
#endif
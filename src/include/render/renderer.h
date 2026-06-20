#ifndef RENDERER_H
#define RENDERER_H

#include <QObject>
#include <QColor>
#include <QString>
#include <Qt>
#include <vector>

class QGraphicsScene;

/// 渲染器 —— 纯绘制原语，不包含任何实体相关的绘制逻辑
///
/// Queue+Flush 模式：behavior 在 tick 中调 queueXxx()，帧末 flush() 一次渲染
class Renderer : public QObject
{
    Q_OBJECT

public:
    explicit Renderer(QGraphicsScene *scene, QObject *parent = nullptr);

    // ========== 帧生命周期 ==========
    void beginFrame(); // 清除上一帧 tag=-1 的临时项
    void flush();      // 一次性渲染队列中所有内容
    void clearAll();   // 清空队列 + 清除临时项（回合结束时调用）

    // ========== Queue 接口（纯原语，不包含实体逻辑）==========

    /// 排队绘制图片（后续 PNG 素材主入口）
    void queueImage(const QString &path, double x, double y,
                    double rotation = 0.0, double scale = 1.0,
                    Qt::Alignment align = Qt::AlignCenter, int z = 100);

    /// 排队绘制实心圆
    void queueCircle(double x, double y, double r,
                     const QColor &color, int z = 100);

    /// 排队绘制实心矩形（支持旋转和居中）
    /// @param centered  true=以(x,y)为中心点; false=(x,y)为左上角
    void queueRect(double x, double y, double w, double h,
                   const QColor &color, int z = 100,
                   double rotationDeg = 0.0, bool centered = false);

    /// 排队绘制文字
    void queueText(const QString &text, double x, double y,
                   const QColor &color, int z = 100);

    /// 排队绘制跳字（带缩放动画）
    void queueSplash(const QString &text, double x, double y,
                     const QColor &color);

private:
    QGraphicsScene *m_scene;

    enum class QueueKind
    {
        Image,
        Circle,
        Rect,
        Text,
        Splash
    };

    struct QueueItem
    {
        QueueKind kind;
        double x = 0, y = 0;
        double param1 = 0; // Image:scale, Circle:radius, Rect:w
        double param2 = 0; // Rect:h
        double rotation = 0;
        QString text;
        QColor color;
        int zValue = 100;
        Qt::Alignment alignment = Qt::AlignCenter;
        bool centered = false;
    };

    std::vector<QueueItem> m_queue;
};

#endif

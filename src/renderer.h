#ifndef RENDERER_H
#define RENDERER_H

#include <QObject>
#include <QColor>
#include <QString>
#include <QFont>
#include <Qt>
#include <vector>
#include <QPixmap>
#include <QMap>
#include <QPen>

// % 渲染通用数据定义

struct TextStyle
{
    QColor color = Qt::white;
    Qt::Alignment alignment = Qt::AlignCenter;
    QFont font = QFont("Microsoft YaHei", 9); // 默认字体和字号
    TextStyle &setColor(const QColor &c)
    {
        color = c;
        return *this;
    }
    TextStyle &setAlign(Qt::Alignment a)
    {
        alignment = a;
        return *this;
    }
    TextStyle &setFont(const QFont &f)
    {
        font = f;
        return *this;
    }
    TextStyle &setFontSize(int size)
    {
        font.setPointSize(size);
        return *this;
    }
    TextStyle &setBold(bool b)
    {
        font.setBold(b);
        return *this;
    }
};

struct LineStyle
{
    QPen pen;
    LineStyle &setPen(const QPen &p)
    {
        pen = p;
        return *this;
    }
    LineStyle &setColor(const QColor &c)
    {
        pen.setColor(c);
        return *this;
    }
    LineStyle &setWidth(qreal w)
    {
        pen.setWidthF(w);
        return *this;
    }
};

// % 各种Payload定义
struct LinePayload
{
    double x, y;
    LineStyle style;
};
struct TextPayload
{
    QString text;
    TextStyle style;
};

struct ImagePayload
{
    QString path;
    double rotation = 0.0;
    double scale = 1.0;
    Qt::Alignment align = Qt::AlignCenter;
};

struct CirclePayload
{
    double radius;
    QColor color;
};

struct RectPayload
{
    double width, height;
    QColor color;
    double rotation = 0.0;
    bool centered = false;
};

struct SplashPayload
{
    QString text;
    TextStyle style;
};

class QGraphicsScene;

// % 渲染队列项
struct QueueItem
{
    double x = 0, y = 0;
    int z = 100;
    std::variant<LinePayload,
                 TextPayload,
                 ImagePayload,
                 CirclePayload,
                 RectPayload,
                 SplashPayload>
        data; // 具体的渲染数据
};
/// % 渲染器
// 仅提供接口
/// Queue+Flush 模式：behavior 在 tick 中调 queueXXX()，帧末 flush() 一次渲染
class Renderer : public QObject
{
    Q_OBJECT

public:
    explicit Renderer(QGraphicsScene *scene, QObject *parent = nullptr);

    // ========== 帧生命周期 ==========
    void beginFrame(); // 清除上一帧 tag=-1 的临时项
    void flush();      // 一次性渲染队列中所有内容
    void clearAll();   // 清空队列 + 清除临时项（回合结束时调用）

    // % Queue 接口

    /// @brief  将线段绘制请求加入队列
    /// @param x1 起始X坐标
    /// @param y1 起始Y坐标
    /// @param x2 终点X坐标
    /// @param y2 终点Y坐标
    /// @param pen 笔刷
    /// @param z Z值
    void queueLine(double x1, double y1, double x2, double y2,
                   const QPen &pen, int z = 100);

    /// @brief 将图片绘制请求加入队列
    /// @param path 图片路径
    /// @param x X坐标
    /// @param y Y坐标
    /// @param rotation 旋转角度
    /// @param scale 缩放比例
    /// @param align 对齐方式
    /// @param z Z值
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

    /// @brief 排队绘制文字
    /// @param text 文本内容
    /// @param x X坐标
    /// @param y Y坐标
    /// @param color 颜色
    /// @param z Z值
    void queueText(const QString &text, double x, double y,
                   const QColor &color, int z = 100);

    /// @brief 排队绘制文字
    /// @param text 文本内容
    /// @param x X坐标
    /// @param y Y坐标
    /// @param style 文字样式
    void queueText(const QString &text, double x, double y,
                   const TextStyle &style, int z = 200);

    /// @brief 排队绘制跳字
    /// @param text 文本内容
    /// @param x X坐标
    /// @param y Y坐标
    /// @param color 颜色
    void queueSplash(const QString &text, double x, double y,
                     const QColor &color, int z = 1000);

    const std::vector<QueueItem> &getQueue() const { return m_queue; }

private:
    QGraphicsScene *m_scene;
    std::vector<QueueItem> m_queue;

    // 图片缓存，避免重复加载
    QMap<QString, QPixmap> m_imageCache;

    /// 从缓存或文件系统加载图片
    QPixmap loadImage(const QString &path);
};

// % 重载透明值运算符
/// @brief 为 QColor 设置 alpha 值的便捷操作符
/// @param color 传入QColor对象
/// @param alpha 传入alpha值（0.0~1.0）
/// @return 修改后的alpha自身
inline QColor operator^(const QColor &color, double alpha)
{
    QColor c = color;
    c.setAlphaF(alpha);
    return c;
}
#endif

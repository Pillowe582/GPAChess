#include "renderer.h"
#include "print.h"

#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QGraphicsRectItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsSimpleTextItem>
#include <QFont>
#include <QPen>
#include <QTimer>
#include <QVariantAnimation>
#include <QRandomGenerator>
#include <algorithm>
#include <cmath>

Renderer::Renderer(QGraphicsScene *scene, QObject *parent)
    : QObject(parent), m_scene(scene)
{
}

// % 图片缓存

QPixmap Renderer::loadImage(const QString &path)
{
    // 检查缓存
    if (m_imageCache.contains(path))
        return m_imageCache[path];

    // 从 Qt 资源系统加载
    QPixmap pix(path);
    if (!pix.isNull())
    {
        m_imageCache[path] = pix;
        return pix;
    }

    print("从文件加载图片失败：" + path);
    // 如果加载失败，返回空 pixmap
    return QPixmap();
}

// % 帧生命周期

void Renderer::beginFrame()
{
    const auto items = m_scene->items();
    for (auto *item : items)
    {
        if (item->data(0).toInt() == -1)
        {
            m_scene->removeItem(item);
            delete item;
        }
    }
}

// % 提交绘制
void Renderer::flush()
{
    for (const auto &item : m_queue)
    {
        std::visit([this, &item](auto &&payload)
                   {
                       using T = std::decay_t<decltype(payload)>;
                       // 渲染圆形
                       if constexpr (std::is_same_v<T, CirclePayload>)
                       {
                           auto *c = m_scene->addEllipse(
                               item.x - payload.radius, item.y - payload.radius,
                               payload.radius * 2.0, payload.radius * 2.0,
                               payload.penEdge, payload.brush);
                           c->setData(0, -1);// 标记为临时项，自动删除
                           c->setZValue(item.z);
                       }
                       // 渲染线
                       else if constexpr (std::is_same_v<T, LinePayload>)
                       {
                           auto *l = m_scene->addLine(
                               item.x, item.y,
                               payload.x, payload.y,
                               payload.style.pen);
                           l->setData(0, -1);// 标记为临时项，自动删除
                       }

                       // 渲染矩形
                       else if constexpr (std::is_same_v<T, RectPayload>){

                           double w = payload.width, h = payload.height;
                           double px = item.x, py = item.y;
                           if (payload.centered)
                           {
                               px -= w / 2.0;
                               py -= h / 2.0;
                           }

                           auto *r = m_scene->addRect(
                               QRectF(px, py, w, h),
                               QPen(Qt::NoPen), QBrush(payload.color));
                           if (payload.rotation != 0.0)
                           {
                               r->setTransformOriginPoint(item.x, item.y);
                               r->setRotation(payload.rotation);
                           }
                           r->setData(0, -1);// 标记为临时项，自动删除
                           r->setZValue(item.z);
                       }

                       // 渲染图片
                       else if constexpr (std::is_same_v<T, ImagePayload>)
                       {
                           // 加载图片
                           QPixmap pix = loadImage(payload.path);
                           if (pix.isNull())
                           {
                               return; // 加载失败，跳过
                           }

                           double scale = payload.scale; // scale
                           double w = 128 * scale;
                           double h = 128 * scale;
                           // 创建 pixmap item（使用 FastTransformation 保持像素边缘清晰）
                           auto scaledPix = pix.scaled(w, h, Qt::KeepAspectRatio, Qt::FastTransformation);

                           double actualW = scaledPix.width();
                            double actualH = scaledPix.height();

                           double px = item.x, py = item.y;
                           if (payload.align & Qt::AlignHCenter)
                               px -= actualW / 2.0;
                           if (payload.align & Qt::AlignVCenter)
                               py -= actualH / 2.0;

                           
                           auto *img = m_scene->addPixmap(scaledPix);
                           img->setPos(px, py);

                           // 支持旋转
                           if (payload.rotation != 0.0)
                           {
                               img->setTransformOriginPoint(actualW / 2.0, actualH / 2.0);
                               img->setRotation(payload.rotation);
                           }

                           img->setData(0, -1);// 标记为临时项，自动删除
                           img->setZValue(item.z);
                       }

                       // 渲染文字
                       else if constexpr (std::is_same_v<T, TextPayload>){
                         auto *text = m_scene->addSimpleText(payload.text);
                         text->setFont(payload.style.font);
                        text->setBrush(payload.style.color);
                        text->setPos(item.x , item.y );
                        text->setZValue(item.z);
                        text->setData(0, -1); // 标记为临时项，自动删除
                       }

                       // 渲染跳字
                       else if constexpr (std::is_same_v<T, SplashPayload>){

                           // 随机初始偏移
                           auto rng = QRandomGenerator::global();
                           double offsetX = (rng->generateDouble() - 0.5) * 30.0;
                           double offsetY = (rng->generateDouble() - 0.5) * 30.0;

                           auto *text = m_scene->addSimpleText(payload.text);
                           text->setFont(payload.style.font);
                           text->setBrush(payload.style.color);
                           text->setPos(item.x - text->boundingRect().width() / 2.0 + offsetX,
                            item.y - 30.0 + offsetY);
                           text->setTransformOriginPoint(text->boundingRect().center());
                           text->setZValue(item.z);
                           text->setScale(4.0);

                           auto *shadow = m_scene->addSimpleText(payload.text);
                           shadow->setFont(payload.style.font);
                           shadow->setBrush(QColor("#000000").darker(160));
                           shadow->setPos(text->pos() + QPointF(2, 2));
                           shadow->setTransformOriginPoint(shadow->boundingRect().center());
                           shadow->setZValue(item.z - 1);
                           shadow->setScale(4.0);

                           auto *anim = new QVariantAnimation(this);
                           anim->setDuration(300);
                           anim->setStartValue(4.0);
                           anim->setEndValue(1.0);
                           anim->setEasingCurve(QEasingCurve::OutCubic);
                           connect(anim, &QVariantAnimation::valueChanged, this,
                                   [text, shadow](const QVariant &val)
                                   {
                                       double s = val.toDouble();
                                       text->setScale(s);
                                       shadow->setScale(s);
                                   });
                           connect(anim, &QVariantAnimation::finished, anim, &QObject::deleteLater);
                           anim->start(QAbstractAnimation::DeleteWhenStopped);

                           QTimer::singleShot(800, this, [text, shadow]()
                                              {
                if (text && text->scene()) text->scene()->removeItem(text);
                if (shadow && shadow->scene()) shadow->scene()->removeItem(shadow);
                delete text;
                delete shadow; });
                           return;
                       } },
                   item.data);
    }
    m_queue.clear();
}

void Renderer::clearAll()
{
    m_queue.clear();
    beginFrame();
}

// % Queue 接口
void Renderer::queueLine(double x1, double y1, double x2, double y2,
                         const QPen &pen, int z)
{
    QueueItem q;
    q.x = x1;
    q.y = y1;
    q.z = z;
    q.data = LinePayload{x2, y2, pen};
    m_queue.push_back(q);
}
void Renderer::queueImage(const QString &path, double x, double y,
                          double rotation, double scale,
                          Qt::Alignment align, int z)
{
    QueueItem q;
    q.x = x;
    q.y = y;
    q.z = z;
    q.data = ImagePayload{path, rotation, scale, align};
    m_queue.push_back(q);
}

void Renderer::queueCircle(double x, double y, double r,
                           const QPen &penEdge, const QBrush &brush, int z)
{
    QueueItem q;
    q.x = x;
    q.y = y;
    q.z = z;
    q.data = CirclePayload{r, penEdge, brush};
    m_queue.push_back(q);
}

void Renderer::queueRect(double x, double y, double w, double h,
                         const QColor &color, int z,
                         double rotationDeg, bool centered)
{
    QueueItem q;
    q.x = x;
    q.y = y;
    q.z = z;
    q.data = RectPayload{w, h, color, rotationDeg, centered};
    m_queue.push_back(q);
}

void Renderer::queueText(const QString &text, double x, double y,
                         const QColor &color, int z)
{
    QueueItem q;
    q.x = x;
    q.y = y;
    q.z = z;
    q.data = TextPayload{text, TextStyle().setColor(color)};
    m_queue.push_back(q);
}

void Renderer::queueText(const QString &text, double x, double y,
                         const TextStyle &style, int z)
{
    QueueItem q;
    q.x = x;
    q.y = y;
    q.z = z;
    q.data = TextPayload{text, style};
    m_queue.push_back(q);
}

void Renderer::queueSplash(const QString &text, double x, double y,
                           const QColor &color, int z)
{
    QueueItem q;
    q.x = x;
    q.y = y;
    q.z = z;
    q.data = SplashPayload{text, TextStyle()
                                     .setColor(color)
                                     .setBold(true)
                                     .setFontSize(18)};
    m_queue.push_back(q);
}

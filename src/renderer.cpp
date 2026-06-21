#include "renderer.h"

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
    : QObject(parent), m_scene(scene),
      color(Qt::white)
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
    for (const auto &q : m_queue)
    {
        switch (q.kind)
        {
        case QueueKind::Circle:
        {
            double r = q.param1;
            auto *c = m_scene->addEllipse(
                q.x - r, q.y - r, r * 2.0, r * 2.0,
                QPen(Qt::NoPen), QBrush(q.color));
            c->setData(0, -1);
            c->setZValue(q.zValue);
            break;
        }
        case QueueKind::Rect:
        {
            double w = q.param1, h = q.param2;
            double px = q.x, py = q.y;
            if (q.centered)
            {
                px -= w / 2.0;
                py -= h / 2.0;
            }

            auto *r = m_scene->addRect(
                QRectF(px, py, w, h),
                QPen(Qt::NoPen), QBrush(q.color));
            if (q.rotation != 0.0)
            {
                r->setTransformOriginPoint(q.x, q.y);
                r->setRotation(q.rotation);
            }
            r->setData(0, -1);
            r->setZValue(q.zValue);
            break;
        }
        case QueueKind::Image:
        {
            // 加载图片
            QPixmap pix = loadImage(q.text);
            if (pix.isNull())
                break; // 加载失败，跳过

            double scale = q.param1; // scale
            double w = 128 * scale;
            double h = 128 * scale;

            double px = q.x, py = q.y;
            if (q.alignment & Qt::AlignHCenter)
                px -= w / 2.0;
            if (q.alignment & Qt::AlignVCenter)
                py -= h / 2.0;

            // 创建 pixmap item（使用 FastTransformation 保持像素边缘清晰）
            auto scaledPix = pix.scaled(w, h, Qt::KeepAspectRatio, Qt::FastTransformation);
            auto *img = m_scene->addPixmap(scaledPix);
            img->setPos(px, py);

            // 支持旋转
            if (q.rotation != 0.0)
            {
                img->setTransformOriginPoint(w / 2.0, h / 2.0);
                img->setRotation(q.rotation);
            }

            img->setData(0, -1);
            img->setZValue(q.zValue);
            break;
        }
        case QueueKind::Text:
        {
            QFont ftFont("Microsoft YaHei", 9);
            ftFont.setBold(true);
            auto *simpleText = m_scene->addSimpleText(q.text);
            simpleText->setFont(ftFont);
            simpleText->setBrush(q.color);
            simpleText->setPos(q.x, q.y);
            simpleText->setData(0, -1);
            simpleText->setZValue(q.zValue);
            break;
        }
        case QueueKind::Splash:
        {
            QFont ftFont("Microsoft YaHei", 20);
            ftFont.setBold(true);

            // 随机初始偏移
            auto rng = QRandomGenerator::global();
            double offsetX = (rng->generateDouble() - 0.5) * 30.0;
            double offsetY = (rng->generateDouble() - 0.5) * 30.0;

            auto *font = m_scene->addSimpleText(q.text);
            font->setFont(ftFont);
            font->setBrush(q.color);
            font->setPos(q.x - font->boundingRect().width() / 2.0 + offsetX, q.y - 30.0 + offsetY);
            font->setTransformOriginPoint(font->boundingRect().center());
            font->setZValue(500);
            font->setScale(4.0);

            auto *shadow = m_scene->addSimpleText(q.text);
            shadow->setFont(ftFont);
            shadow->setBrush(QColor("#000000").darker(160));
            shadow->setPos(font->pos() + QPointF(2, 2));
            shadow->setTransformOriginPoint(shadow->boundingRect().center());
            shadow->setZValue(499);
            shadow->setScale(4.0);

            auto *anim = new QVariantAnimation(this);
            anim->setDuration(300);
            anim->setStartValue(4.0);
            anim->setEndValue(1.0);
            anim->setEasingCurve(QEasingCurve::OutCubic);
            connect(anim, &QVariantAnimation::valueChanged, this,
                    [font, shadow](const QVariant &val)
                    {
                        double s = val.toDouble();
                        font->setScale(s);
                        shadow->setScale(s);
                    });
            connect(anim, &QVariantAnimation::finished, anim, &QObject::deleteLater);
            anim->start(QAbstractAnimation::DeleteWhenStopped);

            QTimer::singleShot(800, this, [font, shadow]()
                               {
                if (font && font->scene()) font->scene()->removeItem(font);
                if (shadow && shadow->scene()) shadow->scene()->removeItem(shadow);
                delete font;
                delete shadow; });
            break;
        }
        }
    }
    m_queue.clear();
}

void Renderer::clearAll()
{
    m_queue.clear();
    beginFrame();
}

// % Queue 接口

void Renderer::queueImage(const QString &path, double x, double y,
                          double rotation, double scale,
                          Qt::Alignment align, int z)
{
    QueueItem q;
    q.kind = QueueKind::Image;
    q.x = x;
    q.y = y;
    q.param1 = scale;
    q.rotation = rotation;
    q.text = path;
    q.color = QColor(180, 180, 180);
    q.zValue = z;
    q.alignment = align;
    m_queue.push_back(q);
}

void Renderer::queueCircle(double x, double y, double r,
                           const QColor &color, int z)
{
    QueueItem q;
    q.kind = QueueKind::Circle;
    q.x = x;
    q.y = y;
    q.param1 = r;
    q.color = color;
    q.zValue = z;
    m_queue.push_back(q);
}

void Renderer::queueRect(double x, double y, double w, double h,
                         const QColor &color, int z,
                         double rotationDeg, bool centered)
{
    QueueItem q;
    q.kind = QueueKind::Rect;
    q.x = x;
    q.y = y;
    q.param1 = w;
    q.param2 = h;
    q.color = color;
    q.zValue = z;
    q.rotation = rotationDeg;
    q.centered = centered;
    m_queue.push_back(q);
}

void Renderer::queueText(const QString &text, double x, double y,
                         const QColor &color, int z)
{
    QueueItem q;
    q.kind = QueueKind::Text;
    q.x = x;
    q.y = y;
    q.text = text;
    q.color = color;
    q.zValue = z;
    m_queue.push_back(q);
}

void Renderer::queueSplash(const QString &text, double x, double y,
                           const QColor &color)
{
    QueueItem q;
    q.kind = QueueKind::Splash;
    q.x = x;
    q.y = y;
    q.text = text;
    q.color = color;
    m_queue.push_back(q);
}

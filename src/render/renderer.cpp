#include "render/renderer.h"

#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QGraphicsRectItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsSimpleTextItem>
#include <QFont>
#include <QPen>
#include <QTimer>
#include <QVariantAnimation>
#include <algorithm>
#include <cmath>

Renderer::Renderer(QGraphicsScene *scene, QObject *parent)
    : QObject(parent), m_scene(scene)
{
}

// ========== 帧生命周期 ==========

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
            double s = q.param1;
            double w = 32.0 * s, h = 32.0 * s;
            double px = q.x, py = q.y;
            if (q.alignment & Qt::AlignHCenter)
                px -= w / 2.0;
            if (q.alignment & Qt::AlignVCenter)
                py -= h / 2.0;
            auto *img = m_scene->addRect(
                QRectF(px, py, w, h),
                QPen(Qt::NoPen), QBrush(q.color));
            img->setData(0, -1);
            img->setZValue(q.zValue);
            break;
        }
        case QueueKind::Text:
        {
            auto *t = m_scene->addSimpleText(q.text);
            t->setBrush(q.color);
            t->setPos(q.x, q.y);
            t->setData(0, -1);
            t->setZValue(q.zValue);
            break;
        }
        case QueueKind::Splash:
        {
            QFont ftFont("Microsoft YaHei", 20);
            ftFont.setBold(true);

            auto *font = m_scene->addSimpleText(q.text);
            font->setFont(ftFont);
            font->setBrush(q.color);
            font->setPos(q.x - font->boundingRect().width() / 2.0, q.y - 30.0);
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

// ========== Queue 接口 ==========

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

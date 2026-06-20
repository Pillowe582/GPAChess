#include "unit_graphics_item.h"

#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QFont>
#include <QtMath>

UnitGraphicsItem::UnitGraphicsItem(int unitUuid, QGraphicsItem *parent)
    : QGraphicsObject(parent), m_uuid(unitUuid)
{
    setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable |
             QGraphicsItem::ItemSendsGeometryChanges);
    setAcceptedMouseButtons(Qt::LeftButton);
    setCacheMode(DeviceCoordinateCache);
}

// --------------- visual ---------------

void UnitGraphicsItem::updateVisual(const QString &name, int currentHp, int maxHp,
                                    const QColor &fillColor, double radius, int starLevel, bool deployed)
{
    if (m_name != name || m_fillColor != fillColor || m_radius != radius ||
        m_currentHp != currentHp || m_maxHp != maxHp || m_starLevel != starLevel || m_deployed != deployed)
    {
        prepareGeometryChange();
        m_name = name;
        m_fillColor = fillColor;
        m_radius = radius;
        m_currentHp = currentHp;
        m_maxHp = maxHp;
        m_starLevel = starLevel;
        m_deployed = deployed;
        update();
    }
}

void UnitGraphicsItem::setDraggable(bool enabled)
{
    m_draggable = enabled;
    // ItemIsMovable 标记控制 Qt 内置移动行为，但我们自己处理拖拽
    setFlag(QGraphicsItem::ItemIsMovable, enabled);
}

// --------------- geometry ---------------

QRectF UnitGraphicsItem::boundingRect() const
{
    // 容纳：上方血条 + 文字区 + 身体圆 + 下方星级（扩大间距）
    const double hBarTop = -m_radius - 36.0;
    const double starBottom = m_radius + 26.0;
    const double side = m_radius * 3.2;
    return QRectF(-side / 2.0, hBarTop, side, starBottom - hBarTop);
}

// --------------- painting ---------------

void UnitGraphicsItem::paint(QPainter *painter, const QStyleOptionGraphicsItem * /*option*/,
                             QWidget * /*widget*/)
{
    painter->setRenderHint(QPainter::Antialiasing);

    // -- 身体圆 --
    QRectF bodyRect(-m_radius, -m_radius, m_radius * 2.0, m_radius * 2.0);
    painter->setPen(QPen(Qt::black, 2.0));
    painter->setBrush(QBrush(m_fillColor));
    painter->drawEllipse(bodyRect);

    // -- 血条（更薄更靠上） --
    const double barWidth = m_radius * 3.2;
    const double barHeight = 8.0;
    const double barX = -barWidth / 2.0;
    const double barY = -m_radius - 30.0;

    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(40, 40, 40, 220));
    painter->drawRect(QRectF(barX, barY, barWidth, barHeight));

    const double hpRatio = m_maxHp > 0
                               ? qBound(0.0, static_cast<double>(m_currentHp) / static_cast<double>(m_maxHp), 1.0)
                               : 0.0;
    painter->setBrush(QColor(70, 220, 90));
    painter->drawRect(QRectF(barX, barY, barWidth * hpRatio, barHeight));

    const QString label = QString("%1\n%2/%3").arg(m_name).arg(m_currentHp).arg(m_maxHp);
    QRectF textRect(barX, barY + barHeight + 4.0, barWidth, 48.0);

    // 阴影辅助：用黑色/深色偏移绘制3层再画前景
    auto drawShadowText = [&](const QRectF &r, int flags, const QColor &fg)
    {
        painter->setPen(QColor(0, 0, 0, 180));
        painter->drawText(r.translated(1, 1), flags, label);
        painter->drawText(r.translated(2, 1), flags, label);
        painter->setPen(fg);
        painter->drawText(r, flags, label);
    };

    QFont font("Microsoft YaHei", 9);
    font.setBold(true);
    painter->setFont(font);
    drawShadowText(textRect, Qt::AlignHCenter | Qt::AlignTop, QColor(255, 255, 255));

    // -- 星级（身体下方，金色，带阴影） --
    if (m_starLevel > 0)
    {
        const QString starStr = QString("%1★").arg(m_starLevel);
        QFont starFont("Microsoft YaHei", 9);
        starFont.setBold(true);
        painter->setFont(starFont);
        QRectF starRect(barX, m_radius + 6.0, barWidth, 20.0);
        painter->setPen(QColor(0, 0, 0, 180));
        painter->drawText(starRect.translated(1, 1), Qt::AlignHCenter | Qt::AlignTop, starStr);
        painter->drawText(starRect.translated(2, 1), Qt::AlignHCenter | Qt::AlignTop, starStr);
        painter->setPen(QColor(255, 215, 0));
        painter->drawText(starRect, Qt::AlignHCenter | Qt::AlignTop, starStr);
    }
}

// --------------- mouse ---------------

void UnitGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (!m_draggable)
    {
        event->ignore();
        return;
    }
    m_dragOrigin = pos();
    setZValue(100); // 拖拽时置顶
    emit dragStarted(m_uuid, m_dragOrigin);
    QGraphicsObject::mousePressEvent(event);
}

void UnitGraphicsItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (!m_draggable)
    {
        event->ignore();
        return;
    }
    QGraphicsObject::mouseMoveEvent(event);
}

void UnitGraphicsItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    setZValue(10); // 恢复层级
    if (!m_draggable)
    {
        event->ignore();
        return;
    }
    QGraphicsObject::mouseReleaseEvent(event);
    emit dragFinished(m_uuid, scenePos());
}

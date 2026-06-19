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
                                    const QColor &fillColor, double radius)
{
    if (m_name != name || m_fillColor != fillColor || m_radius != radius ||
        m_currentHp != currentHp || m_maxHp != maxHp)
    {
        prepareGeometryChange();
        m_name = name;
        m_fillColor = fillColor;
        m_radius = radius;
        m_currentHp = currentHp;
        m_maxHp = maxHp;
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
    // 容纳：上方血条 + 文字区 + 身体圆
    const double hBarTop = -m_radius - 24.0;
    const double bodyBottom = m_radius + 4.0;
    const double side = m_radius * 2.4;
    return QRectF(-side / 2.0, hBarTop, side, bodyBottom - hBarTop);
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

    // -- 血条 --
    const double barWidth = m_radius * 2.4;
    const double barHeight = 10.0;
    const double barX = -barWidth / 2.0;
    const double barY = -m_radius - 20.0;

    // 背景
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(40, 40, 40, 220));
    painter->drawRect(QRectF(barX, barY, barWidth, barHeight));

    // 血量
    const double hpRatio = m_maxHp > 0
                               ? qBound(0.0, static_cast<double>(m_currentHp) / static_cast<double>(m_maxHp), 1.0)
                               : 0.0;
    painter->setBrush(QColor(70, 220, 90));
    painter->drawRect(QRectF(barX, barY, barWidth * hpRatio, barHeight));

    // -- 名字 & HP 文字（放在血条下方，身体上方） --
    painter->setPen(Qt::white);
    QFont font("Microsoft YaHei", 10);
    font.setBold(true);
    painter->setFont(font);

    const QString label = QString("%1\n%2/%3").arg(m_name).arg(m_currentHp).arg(m_maxHp);
    QRectF textRect(barX, barY + barHeight + 2.0, barWidth, 34.0);
    painter->drawText(textRect, Qt::AlignHCenter | Qt::AlignTop, label);
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

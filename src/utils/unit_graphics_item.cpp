#include "unit_graphics_item.h"

#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QFont>
#include <QtMath>
#include "game_manager.h"

UnitGraphicsItem::UnitGraphicsItem(int unitUuid, QGraphicsItem *parent, GameManager *gameManager, Renderer *renderer)
    : QGraphicsObject(parent), m_uuid(unitUuid), m_gameManager(gameManager), m_renderer(renderer)
{
    setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable |
             QGraphicsItem::ItemSendsGeometryChanges);
    setAcceptedMouseButtons(Qt::LeftButton);
    setCacheMode(DeviceCoordinateCache);
}

// --------------- visual ---------------

void UnitGraphicsItem::updateVisual(const QString &, int, int,
                                    const QColor &, double radius, int, bool)
{
    // 仅存储半径（boundingRect / hit test 需要），视觉由 behavior 画
    if (m_radius != radius)
    {
        prepareGeometryChange();
        m_radius = radius;
    }
    update();
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
    if (m_gameManager->getCurrentPhase() != RoundPhase::Prepare)
        return; // 准备阶段由 behavior 画，其他阶段不画（由 entry 直接用 renderer 画）
    const auto *unit = m_gameManager->getPlayerAssets().getUnitByUuid(m_uuid);
    if (!unit || !unit->behavior)
        return;

    unit->behavior->renderSelf(*unit, *painter, 0.5);
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

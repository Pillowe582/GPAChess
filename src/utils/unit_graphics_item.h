#ifndef UNITGRAPHICSITEM_H
#define UNITGRAPHICSITEM_H

#include <QGraphicsObject>
#include <QColor>
#include <QString>
#include <QPointF>
#include "game_manager.h"
#include "renderer.h"

/// 战场上/备战席上的一个可拖拽单位图形项
class UnitGraphicsItem : public QGraphicsObject
{
    Q_OBJECT

public:
    explicit UnitGraphicsItem(int unitUuid, QGraphicsItem *parent = nullptr,
                              GameManager *gameManager = nullptr, Renderer *renderer = nullptr);

    /// 更新视觉外观（HP、名字、颜色、大小、星级）
    void updateVisual(const QString &name, int currentHp, int maxHp,
                      const QColor &fillColor, double radius, int starLevel, bool deployed);

    /// 设置是否可拖拽（准备阶段 = true，其他阶段 = false）
    void setDraggable(bool enabled);

    int unitUuid() const { return m_uuid; }

signals:
    /// 拖拽结束时发射，携带 uuid 和最终 scene 坐标
    void dragFinished(int uuid, QPointF scenePos);
    // 拖拽开始时发射，携带 uuid 和初始 scene 坐标
    void dragStarted(int uuid, QPointF scenePos);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) override;

private:
    int m_uuid;
    bool m_draggable = true;
    QPointF m_dragOrigin;
    GameManager *m_gameManager = nullptr;
    Renderer *m_renderer = nullptr;

    // 仅存储半径（boundingRect 需要），其余视觉由 behavior 负责
    double m_radius = 30.0;
};

#endif // UNITGRAPHICSITEM_H

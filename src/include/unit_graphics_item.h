#ifndef UNITGRAPHICSITEM_H
#define UNITGRAPHICSITEM_H

#include <QGraphicsObject>
#include <QColor>
#include <QString>
#include <QPointF>

/// 战场上/备战席上的一个可拖拽单位图形项
class UnitGraphicsItem : public QGraphicsObject
{
    Q_OBJECT

public:
    explicit UnitGraphicsItem(int unitUuid, QGraphicsItem *parent = nullptr);

    /// 更新视觉外观（HP、名字、颜色、大小）
    void updateVisual(const QString &name, int currentHp, int maxHp,
                      const QColor &fillColor, double radius);

    /// 设置是否可拖拽（准备阶段 = true，其他阶段 = false）
    void setDraggable(bool enabled);

    int unitUuid() const { return m_uuid; }

signals:
    /// 拖拽结束时发射，携带 uuid 和最终 scene 坐标
    void dragFinished(int uuid, QPointF scenePos);

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

    // 视觉参数
    QColor m_fillColor = Qt::gray;
    QString m_name;
    int m_currentHp = 0;
    int m_maxHp = 0;
    double m_radius = 30.0;
};

#endif // UNITGRAPHICSITEM_H

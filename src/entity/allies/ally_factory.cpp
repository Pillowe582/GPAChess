#include "entity/ally_behavior.h"
#include "state.h"
#include "renderer.h"
#include "allies/alpha/alpha_behavior.h"
#include "allies/beta/beta_behavior.h"

#include <QPainter>
#include <QFont>
#include <algorithm>

/// Queue 渲染
void AllyBehavior::renderSelf(const AllyInstance &self, Renderer &renderer,
                              double x, double y)
{
    // 根据 self.name 选择对应的图片
    QString texturePath;
    texturePath = ":/texture/entity/" + self.name + ".png";

    // 缩放比例
    double scale = 0.5;

    // 绘制棋子图片
    renderer.queueImage(texturePath, x, y, 0.0, scale, Qt::AlignCenter, 10);

    int maxHp = self.maxHp.getFinal();
    if (maxHp <= 0)
        return;
    double hpRatio = std::clamp(static_cast<double>(self.currentHp) / maxHp, 0.0, 1.0);

    // 血条尺寸和位置
    double barW = 80.0, barH = 8.0;
    double barX = x - barW / 2.0, barY = y - 55.0; // 调整到图片上方

    renderer.queueRect(barX, barY, barW, barH, QColor("#7b7b7b"), 20);
    renderer.queueRect(barX, barY, barW * hpRatio, barH, QColor("#32CD32"), 21);

    // MP 条（紧贴血条下方 2px，黄色）
    double mpBarH = 4.0;
    double mpBarY = barY + barH + 2.0;
    int maxMp = self.maxMp > 0 ? self.maxMp : 1; // 至少为1避免除以0
    double mpRatio = std::clamp(static_cast<double>(self.currentMp) / maxMp, 0.0, 1.0);
    renderer.queueRect(barX, mpBarY, barW, mpBarH, QColor("#7b7b7b"), 20);
    renderer.queueRect(barX, mpBarY, barW * mpRatio, mpBarH, QColor("#FFD700"), 21);

    double textY = mpBarY + mpBarH + 4.0;
    TextStyle textStyle;
    renderer.queueText(QString("%2/%3").arg(std::ceil(self.currentHp)).arg(maxHp),
                       barX, textY, textStyle.setBold(1).setColor(QColor("#000000") ^ 0.7), 29);
    renderer.queueText(QString("%2/%3").arg(std::ceil(self.currentHp)).arg(maxHp),
                       barX - 2.0, textY - 2.0, textStyle.setBold(1).setColor(QColor("#c2ffc9")), 30);
}

/// @brief 由QPainter自动调用渲染，只用于在准备阶段绘制
/// @param self 棋子
/// @param p QPainter对象
/// @param scale 缩放
void AllyBehavior::renderSelf(const AllyInstance &self, QPainter &p,
                              double scale)
{
    p.setRenderHint(QPainter::Antialiasing);

    // 匹配同名图片资源
    QString texturePath = ":/texture/entity/" + self.name + ".png";

    // 加载并绘制棋子图片
    QPixmap pix(texturePath);
    if (!pix.isNull())
    {
        scale *= self.deployed ? 1.0 : 0.7;
        // 计算缩放比例（适应 scale）
        QPixmap scaledPix = pix.scaled(128 * scale, 128 * scale,
                                       Qt::KeepAspectRatio, Qt::FastTransformation);

        // 居中绘制
        double offsetX = -scaledPix.width() / 2.0;
        double offsetY = -scaledPix.height() / 2.0;
        p.drawPixmap(QPointF(offsetX, offsetY), scaledPix);
    }
    else
    {
        // 如果图片加载失败，回退到圆形
        QColor bodyC = self.deployed ? QColor(60, 110, 255) : QColor(70, 120, 255);
        p.setPen(QPen(Qt::black, 2.0));
        p.setBrush(bodyC);
        p.drawEllipse(QPointF(0, 0), 128 * scale / 2.0, 128 * scale / 2.0);
    }

    // 名字
    double labelX = -50;
    double nameLabelY = -50;
    double starLabelY = 20;
    double labelW = 100;
    QFont font("Microsoft YaHei", 9);
    font.setBold(true);
    p.setFont(font);
    QString label = QString("%1").arg(self.name);
    QRectF textRect(labelX, nameLabelY, labelW, 40.0);

    p.setPen(QColor(0, 0, 0, 180));
    p.drawText(textRect.translated(1, 1), Qt::AlignHCenter | Qt::AlignTop, label);
    p.drawText(textRect.translated(2, 2), Qt::AlignHCenter | Qt::AlignTop, label);
    p.setPen(Qt::white);
    p.drawText(textRect, Qt::AlignHCenter | Qt::AlignTop, label);

    // 星级
    if (self.starLevel > 0)
    {
        QString starStr = QString("%1★").arg(self.starLevel);
        QRectF starRect(labelX, starLabelY, labelW, 20.0);
        p.setPen(QColor(0, 0, 0, 180));
        p.drawText(starRect.translated(2, 2), Qt::AlignHCenter | Qt::AlignTop, starStr);
        p.setPen(QColor(255, 215, 0));
        p.drawText(starRect, Qt::AlignHCenter | Qt::AlignTop, starStr);
    }
}
AllyBehavior *createAllyBehavior(int behaviorId)
{
    switch (behaviorId)
    {
    case 1:
        return new AlphaAlly();
    case 2:
        return new BetaAlly();
    default:
        return new AlphaAlly();
    }
}

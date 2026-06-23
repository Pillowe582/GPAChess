#include "entity/enemy_behavior.h"
#include "state.h"
#include "renderer.h"
#include "enemies/calculus/calculus_behavior.h"
#include "enemies/physics/physics_behavior.h"

#include <algorithm>

/// 敌人渲染：使用纹理图片 + 血条 + HP文字 + MP条
void EnemyBehavior::renderSelf(const EnemyInstance &self, Renderer &renderer,
                               double x, double y)
{

    // 根据 self.id 选择对应的图片
    QString texturePath;
    texturePath = ":/texture/entity/" + self.id + ".png";

    double scale = 0.5; // 缩放比例
    // 绘制敌人图片
    renderer.queueImage(texturePath, x, y, 0.0, scale, Qt::AlignCenter, 10);

    double barW = 80.0, barH = 8.0;
    double barX = x - barW / 2.0, barY = y - 55.0; // 调整到图片上方
    renderer.queueRect(barX, barY, barW, barH, QColor("#7b7b7b"), 20);

    int maxHp = self.maxHp.getFinal();
    double hpRatio = maxHp > 0 ? std::clamp(static_cast<double>(self.currentHp) / maxHp, 0.0, 1.0) : 0.0;
    renderer.queueRect(barX, barY, barW * hpRatio, barH, QColor("#DC3C3C"), 21);

    // MP 条（紧贴血条下方 2px，黄色）
    double mpBarH = 4.0;
    double mpBarY = barY + barH + 2.0;
    int maxMp = self.maxMp > 0 ? self.maxMp : 1; // 至少为1避免除以0
    double mpRatio = std::clamp(static_cast<double>(self.currentMp) / maxMp, 0.0, 1.0);
    renderer.queueRect(barX, mpBarY, barW, mpBarH, QColor("#7b7b7b"), 20);
    renderer.queueRect(barX, mpBarY, barW * mpRatio, mpBarH, QColor("#FFD700"), 21);

    double textY = mpBarY + mpBarH;
    TextStyle textStyle;
    renderer.queueText(QString("%2/%3").arg(std::ceil(self.currentHp)).arg(maxHp),
                       barX, textY, textStyle.setBold(1).setColor(QColor("#000000") ^ 0.7), 29);
    renderer.queueText(QString("%2/%3").arg(std::ceil(self.currentHp)).arg(maxHp),
                       barX - 2.0, textY - 2.0, textStyle.setBold(1).setColor(QColor("#ffc6c6")), 30);
}

EnemyBehavior *createEnemyBehavior(int behaviorId)
{
    switch (behaviorId)
    {
    case 1:
        return new CalculusEnemy();
    case 2:
        return new PhysicsEnemy();
    default:
        return new CalculusEnemy();
    }
}

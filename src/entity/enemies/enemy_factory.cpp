#include "entity/enemy_behavior.h"
#include "state.h"
#include "renderer.h"
#include "enemies/calculus/calculus_behavior.h"
#include "enemies/beta/beta_behavior.h"

#include <algorithm>

/// 敌人渲染：使用纹理图片 + 血条 + 名字/HP
void EnemyBehavior::renderSelf(const EnemyInstance &self, Renderer &renderer,
                               double x = 0, double y = 0)
{

    // 根据 self.name 选择对应的图片
    QString texturePath;
    texturePath = ":/texture/entity/" + self.name + ".png";

    double scale = 0.5; // 缩放比例
    // 绘制敌人图片
    renderer.queueImage(texturePath, x, y, 0.0, scale, Qt::AlignCenter, 10);

    double barW = 80.0, barH = 8.0;
    double barX = x - barW / 2.0, barY = y - 55.0; // 调整到图片上方
    renderer.queueRect(barX, barY, barW, barH, QColor("#7b7b7b"), 20);

    int maxHp = self.maxHp.getFinal();
    double hpRatio = maxHp > 0 ? std::clamp(static_cast<double>(self.currentHp) / maxHp, 0.0, 1.0) : 0.0;
    renderer.queueRect(barX, barY, barW * hpRatio, barH, QColor("#DC3C3C"), 21);

    renderer.queueText(QString("%2/%3").arg(std::ceil(self.currentHp)).arg(maxHp),
                       barX, barY + barH + 4.0, QColor(0, 0, 0, 160), 29);
    renderer.queueText(QString("%2/%3").arg(std::ceil(self.currentHp)).arg(maxHp),
                       barX - 2.0, barY + barH + 2.0, Qt::white, 30);
}

EnemyBehavior *createEnemyBehavior(int behaviorId)
{
    switch (behaviorId)
    {
    case 1:
        return new CalculusEnemy();
    case 2:
        return new BetaEnemy();
    default:
        return new CalculusEnemy();
    }
}

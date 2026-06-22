#ifndef BASE_BEHAVIOR_H
#define BASE_BEHAVIOR_H

class BaseEntity;
class GameManager;

///   onStart()  — 回合开始 / 首次创建时调用一次
///   onTick()   — 每帧调用（各子类用各自签名）
///   onDelete() — 单位即将被移除时调用
class BaseBehavior
{
public:
    virtual ~BaseBehavior() = default;

    virtual void onStart() {}
    virtual void onDelete() {}

    virtual void tick(double dt,
                      BaseEntity &self,
                      GameManager &gameManager) = 0;
};

#endif

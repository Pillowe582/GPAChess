#include "entity/enemy_behavior.h"
#include "enemies/alpha/alpha_behavior.h"
#include "enemies/beta/beta_behavior.h"

EnemyBehavior *createEnemyBehavior(int behaviorId)
{
    switch (behaviorId)
    {
    case 1:
        return new AlphaEnemy();
    case 2:
        return new BetaEnemy();
    default:
        return new AlphaEnemy();
    }
}

#include "entity/ally_behavior.h"
#include "allies/alpha/alpha_behavior.h"
#include "allies/beta/beta_behavior.h"

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

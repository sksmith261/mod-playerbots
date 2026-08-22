#ifndef PLAYERBOTS_NAXXRAIDPLANS_H
#define PLAYERBOTS_NAXXRAIDPLANS_H

#include "Group.h"
#include "Player.h"
#include "RaidDirector.h"

namespace NaxxRaidPlans
{
    // Returns true when the Four Horsemen encounter is running, in which
    // case `plan` has been filled in for every living bot in the group.
    // Receives the plan with its previous assignments intact so holds and
    // camps can persist across ticks.
    bool BuildFourHorsemen(Player* bot, Group* group, RaidPlan& plan);
}

#endif

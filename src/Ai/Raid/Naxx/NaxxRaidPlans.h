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

    // Returns true while Sapphiron is engaged. Phase 1 is the ground fight,
    // phase 2 the air phase, where every bot is given an ice block to break
    // line of sight behind.
    bool BuildSapphiron(Player* bot, Group* group, RaidPlan& plan);
}

#endif

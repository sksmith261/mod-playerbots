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

    // Baseline plan for every other Naxxramas boss: tanks on the boss,
    // melee in, ranged and healers on a ring sized per encounter. Gives
    // every fight shared perception and a readable plan without thirteen
    // bespoke builders; the specialist actions still run above it.
    bool BuildGeneric(Player* bot, Group* group, RaidPlan& plan);

    // Ring radius the generic builder chose for this encounter, so the
    // executing action does not need its own copy of the table.
    float GenericRing(std::string const& label);
}

#endif

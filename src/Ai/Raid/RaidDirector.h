/*
 * Raid Director — one plan per group, one writer per tick.
 *
 * Bots have no raid leader: forty independent agents each derive the same
 * answer from inputs that are not actually identical. The worst offender is
 * perception — "find target" reads the asking bot's own threat list, so
 * "where is the boss" legitimately answers differently for every bot, and
 * answers nothing at all for anyone who has not hit it yet. Encounters kept
 * failing in three different-looking ways for that one reason.
 *
 * The director inverts it. The plan is computed once per group per tick,
 * from a single vantage point, and every bot reads its own line out of it.
 * Three properties follow:
 *
 *   Perception is shared. Bosses are resolved once, by grid, and published
 *   as GUIDs. No bot ever asks its threat list what fight it is in.
 *
 *   Determinism stops being load-bearing. Elections had to be pure
 *   functions producing identical results forty times over — which is why a
 *   remap that missed one call site silently benched every tank. One writer
 *   can simply decide.
 *
 *   Assignments persist. A plan carries its previous state, so decisions
 *   hold across ticks instead of being re-derived from a world the previous
 *   decision just changed. That is what stops bots turning round mid-room.
 *
 * The plan belongs to the group, not to a leader bot, so there is nothing to
 * lose when a bot dies and no handover to get wrong.
 */
#ifndef PLAYERBOTS_RAIDDIRECTOR_H
#define PLAYERBOTS_RAIDDIRECTOR_H

#include <string>
#include <unordered_map>

#include "Action.h"
#include "ObjectGuid.h"
#include "Player.h"

enum RaidDuty : uint8
{
    RAID_DUTY_NONE = 0,
    RAID_DUTY_TANK,      // holds the assigned boss at the assigned camp
    RAID_DUTY_RESERVE,   // rotated off, shedding stacks, must not re-engage
    RAID_DUTY_DAMAGE,
    RAID_DUTY_HEAL,
    RAID_DUTY_HIDE,      // break line of sight behind the assigned object
};

enum RaidEncounterId : uint32
{
    RAID_ENCOUNTER_NONE = 0,
    RAID_ENCOUNTER_FOUR_HORSEMEN,
    RAID_ENCOUNTER_SAPPHIRON,
};

struct RaidAssignment
{
    uint8 duty = RAID_DUTY_NONE;
    uint32 camp = 0;        // index into the encounter's camp table
    ObjectGuid target;      // what this bot fights, if anything
};

struct RaidPlan
{
    uint32 encounter = RAID_ENCOUNTER_NONE;
    uint32 phase = 0;        // encounter-defined; 1 is always the opening hold
    uint32 updatedMs = 0;
    uint32 engagedMs = 0;    // when this encounter was first seen in combat
    std::unordered_map<ObjectGuid, RaidAssignment> assignments;

    RaidAssignment const* For(ObjectGuid guid) const
    {
        auto it = assignments.find(guid);
        return it == assignments.end() ? nullptr : &it->second;
    }
};

// Drives the director. This must not be gated on anything a single bot can
// fail to perceive: the whole point is to escape per-bot threat lookups, so
// hanging the rebuild off an encounter trigger that itself needs threat
// leaves the plan permanently unbuilt.
class RaidDirectorTickAction : public Action
{
public:
    RaidDirectorTickAction(PlayerbotAI* botAI, std::string const name = "raid director tick")
        : Action(botAI, name) {}
    bool Execute(Event event) override;
};

// `raidplan` in party or raid chat: prints what the director decided.
// Everything that has gone wrong on a coordinated fight so far has been
// plainly visible in this table, and invisible from watching the bots.
class RaidPlanAction : public Action
{
public:
    RaidPlanAction(PlayerbotAI* botAI, std::string const name = "raidplan") : Action(botAI, name) {}
    bool Execute(Event event) override;
};

namespace RaidDirector
{
    // Any bot may call this; only the first one inside each window rebuilds.
    void Tick(Player* bot);

    // The plan for this bot's group, or nullptr when no encounter is running.
    RaidPlan const* Get(Player* bot);

    // Human-readable dump for the `raidplan` chat command.
    std::string Describe(Player* bot);
}

#endif

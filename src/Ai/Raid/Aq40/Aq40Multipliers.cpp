#include "Aq40Multipliers.h"

#include "Aq40Utils.h"
#include "Playerbots.h"

float Aq40StomachFocusMultiplier::GetValue(Action* action)
{
    if (!action)
        return 1.0f;

    if (bot->GetMapId() != RaidAq40::MAP_TEMPLE_OF_AHNQIRAJ ||
        bot->GetPositionZ() > RaidAq40::STOMACH_MAX_Z ||
        bot->GetDistance2d(RaidAq40::STOMACH_X, RaidAq40::STOMACH_Y) > RaidAq40::STOMACH_RANGE_2D)
        return 1.0f;

    // Only while there is still a tentacle to kill — once they are down,
    // nothing may interfere with any action (especially the exit run).
    std::list<Creature*> tentacles;
    bot->GetCreatureListWithEntryInGrid(tentacles, RaidAq40::NPC_FLESH_TENTACLE, 100.0f);
    bool tentacleAlive = false;
    for (Creature* tentacle : tentacles)
        if (tentacle && tentacle->IsAlive())
        {
            tentacleAlive = true;
            break;
        }

    if (!tentacleAlive)
        return 1.0f;

    Unit* target = action->GetTarget();
    if (!target || target == bot)
        return 1.0f;

    // Friendly targets (heals on fellow swallowed bots) stay legal
    // regardless of where they stand.
    if (!bot->IsHostileTo(target))
        return 1.0f;

    return target->GetPositionZ() > RaidAq40::STOMACH_MAX_Z ? 0.0f : 1.0f;
}

float Aq40TwinsDutyMultiplier::GetValue(Action* action)
{
    if (!action)
        return 1.0f;

    // Tanks and healers manage their own targets (tank pickup owns tanks;
    // heal targets are friendly and never match a twin).
    if (PlayerbotAI::IsTank(bot) || PlayerbotAI::IsHeal(bot))
        return 1.0f;

    Unit* veklor = AI_VALUE2(Unit*, "find target", "emperor vek'lor");
    Unit* veknilash = AI_VALUE2(Unit*, "find target", "emperor vek'nilash");
    if (!veklor || !veknilash || !veknilash->IsInCombat())
        return 1.0f;

    Unit* wrongTwin = RaidAq40::IsCasterDps(bot) ? veknilash : veklor;
    if (action->GetTarget() != wrongTwin)
        return 1.0f;

    return 0.0f;
}

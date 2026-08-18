#include "Aq40Triggers.h"

#include "Aq40Utils.h"

bool Aq40InStomachTrigger::IsActive()
{
    if (bot->GetMapId() != RaidAq40::MAP_TEMPLE_OF_AHNQIRAJ || !bot->IsAlive())
        return false;

    if (bot->GetPositionZ() > RaidAq40::STOMACH_MAX_Z)
        return false;

    return bot->GetDistance2d(RaidAq40::STOMACH_X, RaidAq40::STOMACH_Y) < RaidAq40::STOMACH_RANGE_2D;
}

bool Aq40DarkGlareTrigger::IsActive()
{
    if (bot->GetMapId() != RaidAq40::MAP_TEMPLE_OF_AHNQIRAJ || !bot->IsAlive())
        return false;

    // Bots down in the stomach are handled by the stomach logic.
    if (bot->GetPositionZ() <= RaidAq40::STOMACH_MAX_Z)
        return false;

    Unit* eye = AI_VALUE2(Unit*, "find target", "eye of c'thun");
    if (!eye || !eye->IsAlive() || !eye->HasAura(RaidAq40::SPELL_RED_COLORATION))
        return false;

    // The eye's live orientation is the beam angle; react when it is close
    // to this bot's angular position around the eye.
    float const delta = RaidAq40::AngleDelta(eye->GetAngle(bot), eye->GetOrientation());
    return std::fabs(delta) < RaidAq40::DARK_GLARE_DANGER_ARC;
}

bool Aq40SarturaWhirlwindTrigger::IsActive()
{
    if (bot->GetMapId() != RaidAq40::MAP_TEMPLE_OF_AHNQIRAJ || !bot->IsAlive())
        return false;

    if (!AI_VALUE2(Unit*, "find target", "battleguard sartura"))
        return false;

    for (auto const& guid : AI_VALUE(GuidVector, "attackers"))
    {
        Unit* unit = botAI->GetUnit(guid);
        if (!unit || !unit->IsAlive() || bot->GetDistance(unit) > RaidAq40::WHIRLWIND_DANGER_RANGE)
            continue;

        if (unit->HasAura(RaidAq40::SPELL_SARTURA_WHIRLWIND) || unit->HasAura(RaidAq40::SPELL_GUARD_WHIRLWIND))
            return true;
    }

    return false;
}

bool Aq40TwinsWrongTargetTrigger::IsActive()
{
    if (bot->GetMapId() != RaidAq40::MAP_TEMPLE_OF_AHNQIRAJ || !bot->IsAlive())
        return false;

    // Tanks and healers manage their own targets.
    if (PlayerbotAI::IsTank(bot) || PlayerbotAI::IsHeal(bot))
        return false;

    Unit* veklor = AI_VALUE2(Unit*, "find target", "emperor vek'lor");
    Unit* veknilash = AI_VALUE2(Unit*, "find target", "emperor vek'nilash");
    Unit* current = AI_VALUE(Unit*, "current target");
    if (!current)
        return false;

    // Only correct bots aimed at the twin they cannot hurt; adds and bugs
    // are legitimate targets.
    if (RaidAq40::IsCasterDps(bot))
        return veklor && current == veknilash;

    return veknilash && current == veklor;
}

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

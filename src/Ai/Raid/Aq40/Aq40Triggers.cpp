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

#include "NaxxActions.h"

#include "NaxxBossHelper.h"
#include "Playerbots.h"

bool FaerlinaWorshipperDutyAction::Execute(Event /*event*/)
{
    Unit* faerlina = AI_VALUE2(Unit*, "find target", "grand widow faerlina");
    if (!faerlina)
        return false;

    std::vector<Unit*> const worshippers = NaxxHelpers::FaerlinaWorshippers(botAI);
    if (worshippers.empty())
        return false;

    // Which assist tank am I? Slot k holds worshipper k (mod count).
    int32 myIndex = -1;
    for (uint8 i = 0; i < 4; ++i)
        if (PlayerbotAI::IsAssistTankOfIndex(bot, i))
        {
            myIndex = i;
            break;
        }

    if (myIndex < 0)
        return false;

    Unit* mine = worshippers[myIndex % worshippers.size()];

    if (AI_VALUE(Unit*, "current target") != mine)
        return Attack(mine);

    // Held and too far from the boss for the death-cast to land: drag it in.
    if (mine->GetVictim() == bot && bot->GetDistance(faerlina) > 18.0f)
        return MoveNear(faerlina, 12.0f, MovementPriority::MOVEMENT_COMBAT);

    return false;
}

bool FaerlinaSacrificeAction::Execute(Event /*event*/)
{
    Unit* faerlina = AI_VALUE2(Unit*, "find target", "grand widow faerlina");
    if (!faerlina)
        return false;

    Unit* sacrifice = NaxxHelpers::FaerlinaSacrificeTarget(botAI, faerlina);
    if (!sacrifice)
        return false;

    if (AI_VALUE(Unit*, "current target") != sacrifice)
        return Attack(sacrifice);

    return false;
}

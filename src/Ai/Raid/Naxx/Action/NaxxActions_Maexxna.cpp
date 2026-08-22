#include "NaxxActions.h"

#include "NaxxBossHelper.h"
#include "Playerbots.h"

bool MaexxnaFreeWrappedAction::Execute(Event /*event*/)
{
    // Wraps pin raiders to the west wall until destroyed. Name-keyed so it
    // works on both the wotlk entry and IP's naxx-40 clone; rescuer index
    // splits multiple wraps between the designated ranged DPS.
    std::vector<Unit*> wraps = NaxxHelpers::MaexxnaWebWraps(botAI, bot);
    if (wraps.empty())
        return false;

    int32 myIndex = 0;
    for (uint8 i = 0; i < 4; ++i)
        if (botAI->IsAssistRangedDpsOfIndex(bot, i))
        {
            myIndex = i;
            break;
        }

    Unit* target = wraps[myIndex % wraps.size()];
    if (AI_VALUE(Unit*, "current target") != target)
        return Attack(target);

    return false;
}

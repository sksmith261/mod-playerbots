#include "NaxxActions.h"

#include "NaxxBossHelper.h"
#include "Playerbots.h"

bool MaexxnaFreeWrappedAction::Execute(Event /*event*/)
{
    // Wraps pin raiders to the west wall until destroyed. Rescuer index
    // splits multiple wraps between the designated ranged DPS.
    std::vector<Creature*> wraps;
    Creature* nearest = bot->FindNearestCreature(NaxxHelpers::NPC_WEB_WRAP, 150.0f);
    if (!nearest)
        return false;

    std::list<Creature*> found;
    bot->GetCreatureListWithEntryInGrid(found, NaxxHelpers::NPC_WEB_WRAP, 150.0f);
    for (Creature* wrap : found)
        if (wrap && wrap->IsAlive())
            wraps.push_back(wrap);

    if (wraps.empty())
        return false;

    std::sort(wraps.begin(), wraps.end(),
              [](Creature* a, Creature* b) { return a->GetGUID() < b->GetGUID(); });

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

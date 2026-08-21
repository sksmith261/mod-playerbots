#include "NaxxActions.h"

#include "NaxxBossHelper.h"
#include "Playerbots.h"

bool NothChooseTargetAction::Execute(Event /*event*/)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "noth the plaguebringer");

    std::vector<Unit*> adds;
    for (auto const& guid : AI_VALUE(GuidVector, "attackers"))
    {
        Unit* unit = botAI->GetUnit(guid);
        if (unit && unit->IsAlive() && NaxxHelpers::IsNothAdd(botAI, unit))
            adds.push_back(unit);
    }

    Unit* target = nullptr;
    if (botAI->IsAssistTank(bot))
    {
        // First untanked skeleton in a stable order.
        std::sort(adds.begin(), adds.end(),
                  [](Unit* a, Unit* b) { return a->GetGUID() < b->GetGUID(); });
        for (Unit* add : adds)
        {
            bool const tanked = add->GetVictim() && add->GetVictim()->ToPlayer() &&
                                PlayerbotAI::IsTank(add->GetVictim()->ToPlayer());
            if (!target)
                target = add;
            if (!tanked)
            {
                target = add;
                break;
            }
        }
        if (!target)
            target = boss;
    }
    else
    {
        // Adds die first (they heal Noth's damage output problem, and the
        // balcony phase is nothing but adds); lowest health finishes fastest.
        for (Unit* add : adds)
            if (!target || add->GetHealthPct() < target->GetHealthPct())
                target = add;

        if (!target)
            target = boss;
    }

    if (!target || AI_VALUE(Unit*, "current target") == target)
        return false;

    return Attack(target);
}

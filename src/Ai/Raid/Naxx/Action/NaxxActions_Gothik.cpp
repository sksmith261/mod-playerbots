#include "NaxxActions.h"

#include "NaxxBossHelper.h"
#include "Playerbots.h"

// Fight-time discipline only. The pre-pull live/dead side split is a raid
// leader decision made with the ground-click tools (@group5-8 goto onto the
// dead side); each side's bots then fight what engages them — threat-based
// "attackers" never crosses the gate.
bool GothikChooseTargetAction::Execute(Event /*event*/)
{
    Unit* gothik = AI_VALUE2(Unit*, "find target", "gothik the harvester");

    // Phase split by altitude: the balcony is z~285, the floor z~268, and
    // the core leaves him ATTACKABLE up there (SetImmuneToPC(false)) — so
    // an attackability check cannot distinguish the phases, and the raid
    // was observed wasting the whole of phase one plinking him on the
    // platform. He is only a real target once he has come down.
    bool const onBalcony = gothik && gothik->GetPositionZ() > 280.0f;

    // Phase two: he is down — burn the boss, adds die to cleave.
    if (gothik && gothik->IsAlive() && !onBalcony)
    {
        if (AI_VALUE(Unit*, "current target") != gothik)
            return Attack(gothik);
        return false;
    }

    std::vector<Unit*> adds;
    for (auto const& guid : AI_VALUE(GuidVector, "attackers"))
    {
        Unit* unit = botAI->GetUnit(guid);
        if (unit && unit->IsAlive() && NaxxHelpers::GothikAddRank(botAI, unit) > 0)
            adds.push_back(unit);
    }

    if (adds.empty())
        return false;

    Unit* target = nullptr;
    if (botAI->IsAssistTank(bot))
    {
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
    }
    else
    {
        // Riders first, then death knights, then trainees; ties by health.
        for (Unit* add : adds)
        {
            if (!target)
            {
                target = add;
                continue;
            }

            int32 const rankNew = NaxxHelpers::GothikAddRank(botAI, add);
            int32 const rankCur = NaxxHelpers::GothikAddRank(botAI, target);
            if (rankNew > rankCur ||
                (rankNew == rankCur && add->GetHealthPct() < target->GetHealthPct()))
                target = add;
        }
    }

    if (!target || AI_VALUE(Unit*, "current target") == target)
        return false;

    return Attack(target);
}

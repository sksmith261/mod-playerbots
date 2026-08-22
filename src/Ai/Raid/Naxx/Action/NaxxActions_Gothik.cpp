#include "NaxxActions.h"

#include "NaxxBossHelper.h"
#include "Playerbots.h"

// Side discipline is the whole fight. Each half of the raid holds its own
// side of the gate and kills what spawns there; nothing may target across
// the gate while it is shut, because it cannot be reached. The pre-pull
// split itself stays a raid-leader call (@group... sweep onto the dead
// side) — bots cannot position before anything has engaged.
bool GothikChooseTargetAction::Execute(Event /*event*/)
{
    Unit* gothik = NaxxHelpers::FindGothik(botAI, bot);
    bool const mySide = NaxxHelpers::GothikLiveSide(bot);
    bool const gateOpen = NaxxHelpers::GothikGateOpen(gothik);

    // Adds on MY side only, unless the gate has opened and the room is one
    // space again. Grid-based so a bot with no threat yet still helps.
    Unit* target = nullptr;
    int32 bestRank = 0;
    for (auto const& guid : AI_VALUE(GuidVector, "possible targets no los"))
    {
        Unit* unit = botAI->GetUnit(guid);
        if (!unit || !unit->IsAlive())
            continue;

        int32 const rank = NaxxHelpers::GothikAddRank(botAI, unit);
        if (rank <= 0)
            continue;

        if (!gateOpen && NaxxHelpers::GothikLiveSide(unit) != mySide)
            continue;

        // Riders before death knights before trainees; then lowest health.
        if (!target || rank > bestRank ||
            (rank == bestRank && unit->GetHealthPct() < target->GetHealthPct()))
        {
            target = unit;
            bestRank = rank;
        }
    }

    // No adds left here: take the boss, but only once he is actually
    // fightable — aggressive, and on this side of a gate we can cross.
    if (!target && gothik && gothik->IsAlive() && !NaxxHelpers::GothikWavePhase(gothik) &&
        (gateOpen || NaxxHelpers::GothikLiveSide(gothik) == mySide))
        target = gothik;

    if (!target || AI_VALUE(Unit*, "current target") == target)
        return false;

    return Attack(target);
}

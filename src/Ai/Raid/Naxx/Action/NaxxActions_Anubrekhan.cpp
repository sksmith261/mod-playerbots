#include "ObjectGuid.h"
#include "Playerbots.h"
#include "NaxxActions.h"
#include "NaxxSpellIds.h"
#include "Spell.h"

bool AnubrekhanChooseTargetAction::Execute(Event /*event*/)
{
    GuidVector attackers = context->GetValue<GuidVector>("attackers")->Get();
    Unit* target = nullptr;
    Unit* target_boss = nullptr;
    std::vector<Unit*> target_guards;
    for (ObjectGuid const guid : attackers)
    {
        Unit* unit = botAI->GetUnit(guid);
        if (!unit)
            continue;
        if (botAI->EqualLowercaseName(unit->GetName(), "crypt guard"))
            target_guards.push_back(unit);

        if (botAI->EqualLowercaseName(unit->GetName(), "anub'rekhan"))
            target_boss = unit;
    }
    if (botAI->IsMainTank(bot))
        target = target_boss;
    else
    {
        if (target_guards.size() == 0)
            target = target_boss;
        else
        {
            if (botAI->IsAssistTank(bot))
            {
                // First untanked guard wins; the old loop inspected the
                // current pick's victim and could settle on a tanked one.
                for (Unit* t : target_guards)
                {
                    if (!target)
                        target = t;

                    bool const tanked = t->GetVictim() && t->GetVictim()->ToPlayer() &&
                                        botAI->IsTank(t->GetVictim()->ToPlayer());
                    if (!tanked)
                    {
                        target = t;
                        break;
                    }
                }
            }
            else
            {
                for (Unit* t : target_guards)
                {
                    if (target == nullptr || target->GetHealthPct() > t->GetHealthPct())
                        target = t;
                }
            }
        }
    }
    if (context->GetValue<Unit*>("current target")->Get() == target)
        return false;

    return Attack(target);
}

bool AnubrekhanPositionAction::Execute(Event /*event*/)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "anub'rekhan");
    if (!boss)
        return false;

    Spell* current = boss->GetCurrentSpell(CURRENT_GENERIC_SPELL);
    bool const castingSwarm =
        current && NaxxSpellIds::MatchesAnySpellId(current->GetSpellInfo(),
            {NaxxSpellIds::LocustSwarm10, NaxxSpellIds::LocustSwarm10Alt, NaxxSpellIds::LocustSwarm25});
    // The old check treated ANY generic cast (Impale included) as the swarm.
    bool inPhase = botAI->HasAura("locust swarm", boss) || castingSwarm;
    if (inPhase)
    {
        if (botAI->IsMainTank(bot))
        {
            uint32 nearest = FindNearestWaypoint();
            uint32 next_point;
            next_point = (nearest + 1) % intervals;

            return MoveTo(bot->GetMapId(), waypoints[next_point].first, waypoints[next_point].second,
                          bot->GetPositionZ(), false, false, false, false, MovementPriority::MOVEMENT_COMBAT);
        }
        else
            return MoveInside(533, 3272.49f, -3476.27f, bot->GetPositionZ(), 3.0f, MovementPriority::MOVEMENT_COMBAT);
    }
    return false;
}

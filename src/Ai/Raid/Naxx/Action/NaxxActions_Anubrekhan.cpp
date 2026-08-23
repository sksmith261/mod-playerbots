#include "ObjectGuid.h"
#include "Playerbots.h"

#include <cmath>
#include "NaxxActions.h"
#include "NaxxSpellIds.h"
#include "Spell.h"
#include "NaxxBossHelper.h"

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

bool AnubrekhanSpreadAction::Execute(Event /*event*/)
{
    float x, y;
    NaxxHelpers::AnubrekhanSpreadSlot(botAI, bot, x, y);
    float z = bot->GetPositionZ();
    bot->UpdateAllowedPositionZ(x, y, z);
    return MoveInside(bot->GetMapId(), x, y, z, 3.0f, MovementPriority::MOVEMENT_COMBAT);
}

bool AnubrekhanPositionAction::Execute(Event /*event*/)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "anub'rekhan");
    if (!boss)
        return false;

    // The old check treated ANY generic cast (Impale included) as the swarm.
    bool const inPhase = NaxxHelpers::AnubrekhanSwarmActive(botAI, boss);
    if (!inPhase)
        return false;

    // Anchor the kite ring on Anub'Rekhan's own spawn. It was hardcoded
    // around a point 36-44y away with a 45y radius, which put waypoints up
    // to 89y out — well through the walls of a room about 50y across, the
    // same defect that was walking Grobbulus out of his room. Radius drops
    // to 22y so the kite stays on the floor.
    Creature* creature = boss->ToCreature();
    if (creature)
    {
        Position const& home = creature->GetHomePosition();
        if (std::fabs(home.GetPositionX() - center_x) > 1.0f ||
            std::fabs(home.GetPositionY() - center_y) > 1.0f)
        {
            center_x = home.GetPositionX();
            center_y = home.GetPositionY();

            waypoints.clear();
            for (uint32 i = 0; i < intervals; ++i)
            {
                float const angle = 2.0f * float(M_PI) * i / intervals;
                waypoints.push_back(std::make_pair(center_x + std::cos(angle) * radius,
                                                   center_y + std::sin(angle) * radius));
            }
        }
    }

    if (botAI->IsMainTank(bot))
    {
        uint32 const next_point = (FindNearestWaypoint() + 1) % intervals;
        return MoveTo(bot->GetMapId(), waypoints[next_point].first, waypoints[next_point].second,
                      bot->GetPositionZ(), false, false, false, false, MovementPriority::MOVEMENT_COMBAT);
    }

    // Everyone else clears the swarm: away from him, not to a fixed point.
    return MoveAway(boss, 25.0f);
}

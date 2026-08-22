#include "NaxxActions.h"
#include "NaxxBossHelper.h"

#include "Playerbots.h"

bool FourHorsemenAttractAlternativelyAction::Execute(Event /*event*/)
{
    if (!helper.UpdateBossAI())
        return false;

    helper.CalculatePosToGo(bot);
    auto [posX, posY] = helper.CurrentAttractPos();
    if (MoveTo(bot->GetMapId(), posX, posY, helper.posZ, false, false, false, false, MovementPriority::MOVEMENT_COMBAT))
        return true;

    Unit* attackTarget = helper.CurrentAttackTarget();
    if (attackTarget && context->GetValue<Unit*>("current target")->Get() != attackTarget)
        return Attack(attackTarget);

    return false;
}

bool FourHorsemenAttackInOrderAction::Execute(Event /*event*/)
{
    if (!helper.UpdateBossAI())
        return false;

    Unit* target = nullptr;
    Unit* thane = AI_VALUE2(Unit*, "find target", "thane korth'azz");
    Unit* lady = AI_VALUE2(Unit*, "find target", "lady blaumeux");
    Unit* sir = AI_VALUE2(Unit*, "find target", "sir zeliek");
    Unit* fourth = AI_VALUE2(Unit*, "find target", "baron rivendare");
    if (!fourth)
        fourth = AI_VALUE2(Unit*, "find target", "highlord mograine");

    std::vector<Unit*> attack_order;
    if (botAI->IsAssistTank(bot))
        attack_order = {fourth, thane, lady, sir};
    else
        attack_order = {thane, fourth, lady, sir};
    for (Unit* t : attack_order)
    {
        if (t && t->IsAlive())
        {
            target = t;
            break;
        }
    }
    if (target)
    {
        if (context->GetValue<Unit*>("current target")->Get() == target && botAI->GetState() == BOT_STATE_COMBAT)
            return false;

        if (!bot->IsWithinLOSInMap(target))
            return MoveNear(target, 22.0f, MovementPriority::MOVEMENT_COMBAT);

        return Attack(target);
    }
    return false;
}

bool FourHorsemenDutyAction::Execute(Event /*event*/)
{
    // Corner stations = the four quadrants of the chamber. IP's horsemen
    // do not walk to them on their own (that script has no waypoints), so
    // the tanks drag them out; separating them is what keeps a raider from
    // collecting all four Marks at once.
    struct Side
    {
        char const* name;
        char const* altName;
        uint32 markId;
        float x, y;
    };
    static Side const front[2] = {
        {"thane korth'azz", nullptr, 28832, 2542.9f, -3015.0f},
        {"highlord mograine", "baron rivendare", 28834, 2583.9f, -2971.6f},
    };
    static Side const back[2] = {
        {"lady blaumeux", nullptr, 28833, 2469.4f, -2947.6f},
        {"sir zeliek", nullptr, 28835, 2517.8f, -2896.6f},
    };

    auto resolve = [&](Side const& side) -> Unit*
    {
        Unit* unit = AI_VALUE2(Unit*, "find target", side.name);
        if (!unit && side.altName)
            unit = AI_VALUE2(Unit*, "find target", side.altName);
        return (unit && unit->IsAlive()) ? unit : nullptr;
    };

    auto taunt = [&](Unit* target)
    {
        switch (bot->getClass())
        {
            case CLASS_DRUID:        botAI->CastSpell("growl", target); break;
            case CLASS_PALADIN:      botAI->CastSpell("hand of reckoning", target); break;
            case CLASS_DEATH_KNIGHT: botAI->CastSpell("dark command", target); break;
            default:                 botAI->CastSpell("taunt", target); break;
        }
    };

    bool const rangedSide = PlayerbotAI::IsRanged(bot) || PlayerbotAI::IsHeal(bot);

    // ---- Tanks: one horseman each, held for the whole fight ----
    // Every Mark halves its target's threat (boss_four_horsemen_40.cpp:
    // SpellHitTarget -> DoModifyThreatByPercent(-50)), so holding one of
    // these is continuous taunt work, not a single pull. Tanks therefore do
    // not join the stack rotation — they stay on their boss.
    int32 slot = -1;
    if (PlayerbotAI::IsTank(bot))
    {
        if (botAI->IsMainTank(bot))
            slot = 0;
        else if (PlayerbotAI::IsAssistTankOfIndex(bot, 0))
            slot = 1;
        else if (PlayerbotAI::IsAssistTankOfIndex(bot, 1))
            slot = 2;
        else if (PlayerbotAI::IsAssistTankOfIndex(bot, 2))
            slot = 3;
    }

    if (slot >= 0)
    {
        Side const& mine = slot < 2 ? front[slot] : back[slot - 2];
        Unit* boss = resolve(mine);
        if (!boss)
            return false;  // mine is dead: the rest of the raid mops up

        // Not holding it? Get on it and taunt. Position comes second —
        // walking to the station first (the old order) left the tank in an
        // empty corner while the boss stayed where it stood, which is
        // exactly the "runs to the corner but never holds aggro" report.
        if (boss->GetVictim() != bot)
        {
            if (!bot->IsWithinMeleeRange(boss))
                return MoveNear(boss, 3.0f, MovementPriority::MOVEMENT_COMBAT);

            taunt(boss);
            if (AI_VALUE(Unit*, "current target") != boss)
                return Attack(boss);

            return false;
        }

        // Holding it: walk to the station and it follows.
        if (bot->GetExactDist2d(mine.x, mine.y) > 6.0f)
        {
            float x = mine.x, y = mine.y, z = bot->GetPositionZ();
            bot->UpdateAllowedPositionZ(x, y, z);
            return MoveInside(bot->GetMapId(), x, y, z, 4.0f, MovementPriority::MOVEMENT_COMBAT);
        }

        if (AI_VALUE(Unit*, "current target") != boss)
            return Attack(boss);

        return false;
    }

    // ---- Everyone else: stack-driven rotation between their pair ----
    Side const* sides = rangedSide ? back : front;

    uint32 rank = 0;
    if (Group* group = bot->GetGroup())
        for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
        {
            Player* member = itr->GetSource();
            if (!member || !member->IsAlive() || !GET_PLAYERBOT_AI(member) || PlayerbotAI::IsTank(member))
                continue;

            bool const memberRanged = PlayerbotAI::IsRanged(member) || PlayerbotAI::IsHeal(member);
            if (memberRanged != rangedSide)
                continue;

            if (member == bot)
                break;

            ++rank;
        }

    // The Mark is the clock: three stacks of this side's mark means cross.
    uint32 side = (rank + (flipped ? 1 : 0)) % 2;
    if (Aura* mark = bot->GetAura(sides[side].markId))
        if (mark->GetStackAmount() >= 3)
        {
            flipped = !flipped;
            side = (side + 1) % 2;
        }

    Unit* boss = resolve(sides[side]);
    if (!boss)
    {
        side = (side + 1) % 2;
        boss = resolve(sides[side]);
        if (!boss)
            return false;
    }

    // Stand off the station toward the room centre so casters keep range.
    float const cx = 2525.0f, cy = -2955.0f;
    float dx = cx - sides[side].x, dy = cy - sides[side].y;
    float const len = std::sqrt(dx * dx + dy * dy);
    float px = sides[side].x + dx / len * 10.0f;
    float py = sides[side].y + dy / len * 10.0f;

    if (bot->GetExactDist2d(px, py) > 8.0f)
    {
        float z = bot->GetPositionZ();
        bot->UpdateAllowedPositionZ(px, py, z);
        return MoveInside(bot->GetMapId(), px, py, z, 4.0f, MovementPriority::MOVEMENT_COMBAT);
    }

    if (PlayerbotAI::IsHeal(bot))
        return false;  // positioned; the heal engine owns the rest

    if (AI_VALUE(Unit*, "current target") != boss)
        return Attack(boss);

    return false;
}

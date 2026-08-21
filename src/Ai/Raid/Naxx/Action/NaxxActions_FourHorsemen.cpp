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
    // Corner stations = each horseman's final scripted waypoint
    // (boss_four_horsemen.cpp WaypointPositions[2/5/8/11]).
    struct Side
    {
        char const* name;
        char const* altName;
        uint32 markId;
        float x, y;
    };
    // Front: Thane (28832) / Baron (28834). Back: Lady (28833) / Sir (28835).
    static Side const front[2] = {
        {"thane korth'azz", nullptr, 28832, 2542.9f, -3015.0f},
        {"baron rivendare", "highlord mograine", 28834, 2583.9f, -2971.6f},
    };
    static Side const back[2] = {
        {"lady blaumeux", nullptr, 28833, 2469.4f, -2947.6f},
        {"sir zeliek", nullptr, 28835, 2517.8f, -2896.6f},
    };

    bool const isFront = PlayerbotAI::IsTank(bot) || (!PlayerbotAI::IsRanged(bot) && !PlayerbotAI::IsHeal(bot));
    Side const* sides = isFront ? front : back;

    // Base parity: main tank side 0, first assist tank side 1; everyone
    // else by rank among living bots of the same team in group order.
    int32 parity = -1;
    if (botAI->IsMainTank(bot))
        parity = 0;
    else if (PlayerbotAI::IsAssistTankOfIndex(bot, 0))
        parity = 1;

    if (parity < 0)
    {
        uint32 rank = 0;
        if (Group* group = bot->GetGroup())
            for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
            {
                Player* member = itr->GetSource();
                if (!member || !member->IsAlive() || !GET_PLAYERBOT_AI(member))
                    continue;

                bool const memberFront = PlayerbotAI::IsTank(member) ||
                                         (!PlayerbotAI::IsRanged(member) && !PlayerbotAI::IsHeal(member));
                if (memberFront != isFront)
                    continue;

                if (member == bot)
                    break;

                ++rank;
            }
        parity = rank % 2;
    }

    // The Mark itself is the rotation clock: 3 stacks of my current side's
    // mark means it is time to cross. The old mark decays while the new
    // side's builds, so the flip re-arms itself naturally.
    uint32 side = (parity + (flipped ? 1 : 0)) % 2;
    if (Aura* mark = bot->GetAura(sides[side].markId))
        if (mark->GetStackAmount() >= 3)
        {
            flipped = !flipped;
            side = (side + 1) % 2;
        }

    Unit* boss = AI_VALUE2(Unit*, "find target", sides[side].name);
    if (!boss && sides[side].altName)
        boss = AI_VALUE2(Unit*, "find target", sides[side].altName);
    if (!boss || !boss->IsAlive())
    {
        // My side's horseman is dead: converge on the pair partner.
        uint32 const other = (side + 1) % 2;
        boss = AI_VALUE2(Unit*, "find target", sides[other].name);
        if (!boss && sides[other].altName)
            boss = AI_VALUE2(Unit*, "find target", sides[other].altName);
        if (!boss || !boss->IsAlive())
            return false;
        side = other;
    }

    // Stand at the station (nudged toward room center so casters have
    // range on a corner-parked boss).
    float const cx = 2525.0f, cy = -2955.0f;
    float dx = cx - sides[side].x, dy = cy - sides[side].y;
    float const len = std::sqrt(dx * dx + dy * dy);
    float const px = sides[side].x + dx / len * 10.0f;
    float const py = sides[side].y + dy / len * 10.0f;

    if (bot->GetExactDist2d(px, py) > 8.0f && (PlayerbotAI::IsRanged(bot) || PlayerbotAI::IsHeal(bot) ||
                                               PlayerbotAI::IsTank(bot)))
    {
        float z = bot->GetPositionZ();
        bot->UpdateAllowedPositionZ(px, py, z);
        return MoveInside(bot->GetMapId(), px, py, z, 4.0f, MovementPriority::MOVEMENT_COMBAT);
    }

    if (PlayerbotAI::IsHeal(bot))
        return false;  // positioned; the heal engine owns the rest

    // Tanks: make the swap crisp — taunt when the side boss is not on us.
    if (PlayerbotAI::IsTank(bot) && boss->GetVictim() && boss->GetVictim() != bot &&
        bot->IsWithinMeleeRange(boss))
    {
        switch (bot->getClass())
        {
            case CLASS_DRUID:   botAI->CastSpell("growl", boss); break;
            case CLASS_PALADIN: botAI->CastSpell("hand of reckoning", boss); break;
            case CLASS_DEATH_KNIGHT: botAI->CastSpell("dark command", boss); break;
            default:            botAI->CastSpell("taunt", boss); break;
        }
    }

    if (AI_VALUE(Unit*, "current target") != boss)
        return Attack(boss);

    return false;
}

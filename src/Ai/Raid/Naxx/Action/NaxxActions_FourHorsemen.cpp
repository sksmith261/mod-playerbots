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
    // Drives this action's own copy of the encounter clock (the trigger
    // holds a separate instance), which the opening damage hold reads.
    if (!helper.UpdateBossAI())
        return false;

    using namespace NaxxHelpers;
    HorsemanSpec const* specs = FourHorsemenSpecs();

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

    auto moveTo2d = [&](float x, float y, float tolerance) -> bool
    {
        if (bot->GetExactDist2d(x, y) <= tolerance)
            return false;

        float z = bot->GetPositionZ();
        bot->UpdateAllowedPositionZ(x, y, z);
        return MoveInside(bot->GetMapId(), x, y, z, tolerance * 0.75f, MovementPriority::MOVEMENT_COMBAT);
    };

    // ---- Tank pool: eight deep, two per horseman -----------------------
    std::vector<Player*> const pool = FourHorsemenTankPool(bot);
    int32 myPoolIndex = -1;
    for (uint32 i = 0; i < pool.size(); ++i)
        if (pool[i] == bot)
            myPoolIndex = int32(i);

    if (myPoolIndex >= 0)
    {
        uint32 const slot = uint32(myPoolIndex) % 4;
        HorsemanSpec const& mine = specs[slot];
        Unit* boss = ResolveHorseman(botAI, mine);
        Player* active = FourHorsemenActiveTank(pool, slot, mine.markId);

        if (boss && active == bot)
        {
            // Marks halve threat on every application, so holding one of
            // these is continuous taunt work. Engage first, position after
            // — walking to the camp first leaves the boss where it stood.
            // The camp is where this horseman lives for the whole fight,
            // so the tank anchors there and does NOT chase. Lost threat is
            // answered with a taunt from the camp; only a boss dragged out
            // of taunt range is worth walking to.
            if (boss->GetVictim() != bot)
            {
                if (bot->GetDistance(boss) > 25.0f)
                    return MoveNear(boss, 20.0f, MovementPriority::MOVEMENT_COMBAT);

                taunt(boss);
            }

            if (moveTo2d(mine.x, mine.y, 4.0f))
                return true;

            if (AI_VALUE(Unit*, "current target") != boss)
                return Attack(boss);

            return false;
        }

        // Rotated off — but the handoff has to COMPLETE before leaving.
        // Walking to the middle while the boss is still on us drags it
        // there and merges it with the other three, which wrecks the whole
        // fight. Hold the camp, keep it pinned, and let the partner taunt
        // it away first.
        if (boss && boss->GetVictim() == bot)
        {
            if (moveTo2d(mine.x, mine.y, 6.0f))
                return true;

            if (AI_VALUE(Unit*, "current target") != boss)
                return Attack(boss);

            return false;
        }

        // Aggro is genuinely off us now: park in the middle until the
        // stacks expire, then rejoin as damage.
        Aura* mark = bot->GetAura(mine.markId);
        if (mark && mark->GetStackAmount() > 0)
        {
            if (moveTo2d(FH_SAFE_X, FH_SAFE_Y, 5.0f))
                return true;

            return false;  // parked and shedding stacks: do not pull anything
        }
    }

    // ---- Everyone else: one camp per role ------------------------------
    // Melee take Korth'azz and Mograine; ranged and healers take Blaumeux
    // and Zeliek, because Holy Wrath chains through anyone in melee of him.
    bool const rangedSide = PlayerbotAI::IsRanged(bot) || PlayerbotAI::IsHeal(bot);
    uint32 const slotA = rangedSide ? 2u : 0u;
    uint32 const slotB = rangedSide ? 3u : 1u;

    uint32 rank = 0;
    if (Group* group = bot->GetGroup())
        for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
        {
            Player* member = itr->GetSource();
            if (!member || !member->IsAlive() || !GET_PLAYERBOT_AI(member))
                continue;

            bool const memberRanged = PlayerbotAI::IsRanged(member) || PlayerbotAI::IsHeal(member);
            if (memberRanged != rangedSide)
                continue;

            if (member == bot)
                break;

            ++rank;
        }

    uint32 slot = (rank % 2) ? slotB : slotA;

    // The Mark is the rotation clock: three stacks and cross to the pair
    // partner, where the other mark builds while this one decays.
    if (Aura* mark = bot->GetAura(specs[slot].markId))
        if (mark->GetStackAmount() >= 3)
            slot = (slot == slotA) ? slotB : slotA;

    Unit* boss = ResolveHorseman(botAI, specs[slot]);
    if (!boss)
    {
        slot = (slot == slotA) ? slotB : slotA;
        boss = ResolveHorseman(botAI, specs[slot]);
        if (!boss)
            return false;
    }

    HorsemanSpec const& camp = specs[slot];
    float x = camp.x, y = camp.y;

    if (slot == 0)
    {
        // Korth'azz: Meteor splits its damage between everyone it lands on,
        // so this camp stacks on one point rather than spreading.
        if (moveTo2d(x, y, 3.0f))
            return true;
    }
    else
    {
        // Everyone else spreads: Void Zones need room to step out of, and
        // Holy Wrath jumps between raiders standing close together, so
        // Zeliek's camp spreads twice as wide as the rest.
        float const spacing = (slot == 3) ? 12.0f : 6.0f;
        float const angle = float(rank / 2 % 6) * (float(M_PI) / 3.0f);
        x += spacing * std::cos(angle);
        y += spacing * std::sin(angle);

        if (moveTo2d(x, y, 4.0f))
            return true;
    }

    if (PlayerbotAI::IsHeal(bot))
        return false;  // in position; the heal engine owns the rest

    // Opening seconds: take the camp, but do not touch the bosses until
    // the tanks have them parked and threatened.
    if (helper.InPullGrace())
        return false;

    if (AI_VALUE(Unit*, "current target") != boss)
        return Attack(boss);

    return false;
}

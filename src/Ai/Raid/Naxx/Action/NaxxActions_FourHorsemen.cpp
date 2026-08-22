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

    // A promoted damage spec cannot simply taunt: a warrior's Taunt needs
    // Defensive Stance and a druid's Growl needs Bear Form. Without the
    // shift the cast silently fails, the horseman is never pulled, and it
    // sits at its spawn for the whole fight — which is precisely what
    // happened to Zeliek, whose tank is the first promoted damage bot.
    auto taunt = [&](Unit* target)
    {
        switch (bot->getClass())
        {
            case CLASS_DRUID:
                if (!bot->HasAura(5487) && !bot->HasAura(9634))
                {
                    botAI->CastSpell("bear form", bot);
                    return;
                }
                botAI->CastSpell("growl", target);
                break;
            case CLASS_WARRIOR:
                if (!bot->HasAura(71))
                {
                    botAI->CastSpell("defensive stance", bot);
                    return;
                }
                botAI->CastSpell("taunt", target);
                break;
            case CLASS_PALADIN:      botAI->CastSpell("hand of reckoning", target); break;
            case CLASS_DEATH_KNIGHT: botAI->CastSpell("dark command", target); break;
            default:                 botAI->CastSpell("taunt", target); break;
        }
    };

    // Pets build threat on whatever they hit, and every Mark halves the
    // tank's, so one loose pet is enough to peel a horseman off its camp
    // and drag it across the room. Pets are held during the opening and
    // then kept strictly on their owner's own horseman. Covers hunter and
    // warlock pets, water elementals, ghouls and other guardians alike.
    auto commandPets = [&](Unit* target)
    {
        for (Unit* controlled : bot->m_Controlled)
        {
            Creature* pet = controlled ? controlled->ToCreature() : nullptr;
            if (!pet || !pet->IsAlive())
                continue;

            CharmInfo* charm = pet->GetCharmInfo();

            if (!target)
            {
                pet->AttackStop();
                pet->SetReactState(REACT_PASSIVE);
                if (charm)
                    charm->SetIsCommandAttack(false);
                continue;
            }

            pet->SetReactState(REACT_DEFENSIVE);
            if (pet->GetVictim() != target)
            {
                if (charm)
                    charm->SetIsCommandAttack(true);
                if (pet->AI())
                    pet->AI()->AttackStart(target);
            }
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
        // Pool order is real tanks first, so pool index maps to horseman
        // through this table rather than directly: with three real tanks
        // they cover Korth'azz, Mograine and Zeliek, and the promoted
        // damage spec — in damage gear, with damage health — gets Blaumeux,
        // who only casts. Sending it to Zeliek instead put the weakest tank
        // in melee of the one boss whose Holy Wrath chains through melee.
        static uint32 const slotForPoolIndex[4] = {0u, 1u, 3u, 2u};
        uint32 const poolGroup = uint32(myPoolIndex) % 4;
        uint32 const slot = slotForPoolIndex[poolGroup];
        HorsemanSpec const& mine = specs[slot];
        Unit* boss = ResolveHorseman(botAI, mine);

        // Elect among the POOL GROUP (indices poolGroup and poolGroup+4),
        // not the horseman slot. Once the two stopped being the same number
        // this election looked at a different pair than the one that had
        // actually been assigned here, so every tank concluded it was the
        // reserve and walked to the safe spot, leaving all four horsemen
        // unheld.
        Player* active = FourHorsemenActiveTank(pool, poolGroup, mine.markId, boss);

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

            commandPets(boss);

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
            commandPets(nullptr);  // nothing follows us to the middle

            if (moveTo2d(FH_SAFE_X, FH_SAFE_Y, 5.0f))
                return true;

            return false;  // parked and shedding stacks: do not pull anything
        }
    }

    // ---- Everyone else: one camp per role ------------------------------
    // Melee take Korth'azz and Mograine; ranged and healers take Blaumeux
    // and Zeliek, because Holy Wrath chains through anyone in melee of him.
    bool const isHealer = PlayerbotAI::IsHeal(bot);
    bool const rangedSide = PlayerbotAI::IsRanged(bot) || isHealer;

    // Rank among bots of the same kind. Healers are counted as their own
    // group so they can be spread independently of the damage split.
    uint32 rank = 0;
    if (Group* group = bot->GetGroup())
        for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
        {
            Player* member = itr->GetSource();
            if (!member || !member->IsAlive() || !GET_PLAYERBOT_AI(member))
                continue;

            bool const memberHealer = PlayerbotAI::IsHeal(member);
            if (memberHealer != isHealer)
                continue;

            if (!isHealer)
            {
                bool const memberRanged = PlayerbotAI::IsRanged(member) || memberHealer;
                if (memberRanged != rangedSide)
                    continue;
            }

            if (member == bot)
                break;

            ++rank;
        }

    // Camp geometry, measured: Korth'azz-Mograine is 60y and Blaumeux-
    // Zeliek 66y (the short sides), Korth'azz-Blaumeux and Mograine-Zeliek
    // are 100y (the long sides), and Korth'azz-Zeliek / Mograine-Blaumeux
    // are 117-119y (the diagonals). Perimeter order is therefore
    // Korth'azz, Mograine, Zeliek, Blaumeux.
    static uint32 const ring[4] = {0u, 1u, 3u, 2u};
    static uint32 const diagonal[4] = {3u, 2u, 1u, 0u};

    uint32 slotA, slotB;
    if (isHealer)
    {
        // Healers rotate clockwise — one camp round the perimeter. Stepping
        // through raw slot indices instead sent half of them across a 117y
        // diagonal, roughly seventeen seconds out of position and gathering
        // marks the whole way, which is what was killing them.
        uint32 const r = rank % 4;
        slotA = ring[r];
        slotB = ring[(r + 1) % 4];
    }
    else if (rangedSide)
    {
        // Ranged damage rotates diagonally: the long way, which drops a
        // mark completely rather than trading it for a neighbour's.
        slotA = (rank % 2) ? 3u : 2u;
        slotB = diagonal[slotA];
    }
    else
    {
        // Melee cannot enter Zeliek's camp at all — Holy Wrath chains
        // through anyone in melee — so they alternate between the only two
        // camps that can hold them, which is also the shortest hop.
        slotA = (rank % 2) ? 1u : 0u;
        slotB = slotA == 0u ? 1u : 0u;
    }

    // Which camp am I actually standing in? Anchoring this to slotA meant
    // only slotA's mark was ever examined: a bot parked at its partner camp
    // accumulated that mark unchecked — five, six stacks — and snapped back
    // the instant the slotA mark lapsed, regardless of what it was carrying.
    uint32 slot = bot->GetExactDist2d(specs[slotA].x, specs[slotA].y) <=
                          bot->GetExactDist2d(specs[slotB].x, specs[slotB].y)
                      ? slotA
                      : slotB;

    // The Mark is the rotation clock: at the threshold, cross to the pair
    // partner, where the other mark builds while this one decays.
    if (Aura* mark = bot->GetAura(specs[slot].markId))
        if (mark->GetStackAmount() >= NaxxHelpers::FH_SWAP_STACKS)
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
    {
        commandPets(nullptr);
        return false;
    }

    // Zeliek's camp is ranged only, and pets are melee. Sending them in
    // feeds Holy Wrath and piles threat onto the most fragile tank in the
    // raid, so they sit this camp out.
    commandPets(slot == 3 ? nullptr : boss);

    if (AI_VALUE(Unit*, "current target") != boss)
        return Attack(boss);

    return false;
}

#include "NaxxActions.h"
#include "NaxxBossHelper.h"
#include "RaidDirector.h"

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

// The decisions — which camp, which boss, tank or reserve — all live in the
// raid plan now. This action only carries them out.
bool FourHorsemenDutyAction::Execute(Event /*event*/)
{
    using namespace NaxxHelpers;

    RaidDirector::Tick(bot);
    RaidPlan const* plan = RaidDirector::Get(bot);
    if (!plan || plan->encounter != RAID_ENCOUNTER_FOUR_HORSEMEN)
        return false;

    RaidAssignment const* mine = plan->For(bot->GetGUID());
    if (!mine || mine->duty == RAID_DUTY_NONE)
        return false;

    HorsemanSpec const* specs = FourHorsemenSpecs();
    HorsemanSpec const& camp = specs[mine->camp];
    Unit* boss = mine->target ? ObjectAccessor::GetUnit(*bot, mine->target) : nullptr;

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

    auto moveTo2d = [&](float x, float y, float tolerance) -> bool
    {
        if (bot->GetExactDist2d(x, y) <= tolerance)
            return false;

        float z = bot->GetPositionZ();
        bot->UpdateAllowedPositionZ(x, y, z);
        return MoveInside(bot->GetMapId(), x, y, z, tolerance * 0.75f, MovementPriority::MOVEMENT_COMBAT);
    };

    // Pets build threat wherever they are sent, and every Mark halves the
    // tank's, so a pet loose in the wrong camp peels a horseman. They go
    // strictly where their owner goes, or nowhere.
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

    // ---- Reserve: rotated off, shedding stacks ---------------------------
    if (mine->duty == RAID_DUTY_RESERVE)
    {
        // Never walk out while the boss is still on us — it follows, and the
        // camps collapse into the middle. Hold until the relief has taunted.
        if (boss && boss->GetVictim() == bot)
        {
            commandPets(boss);
            if (moveTo2d(camp.x, camp.y, 6.0f))
                return true;

            if (AI_VALUE(Unit*, "current target") != boss)
                return Attack(boss);

            return false;
        }

        commandPets(nullptr);
        moveTo2d(FH_SAFE_X, FH_SAFE_Y, 5.0f);

        // Hold the tick even once parked: yielding hands the bot to the
        // generic combat AI, which sends it straight back to the horseman it
        // was just relieved of.
        return true;
    }

    // ---- Tank: anchor the camp, taunt from it ----------------------------
    if (mine->duty == RAID_DUTY_TANK)
    {
        if (!boss)
            return false;

        if (boss->GetVictim() != bot)
        {
            if (bot->GetDistance(boss) > 25.0f)
                return MoveNear(boss, 20.0f, MovementPriority::MOVEMENT_COMBAT);

            taunt(boss);
        }

        commandPets(boss);

        if (moveTo2d(camp.x, camp.y, 4.0f))
            return true;

        if (AI_VALUE(Unit*, "current target") != boss)
            return Attack(boss);

        return false;
    }

    // ---- Damage and healers ---------------------------------------------
    if (mine->camp == 0)
    {
        // Korth'azz: Meteor splits its damage across everyone it lands on,
        // so this camp stacks on one point rather than spreading.
        if (moveTo2d(camp.x, camp.y, 3.0f))
            return true;
    }
    else
    {
        // Void Zones need room to step out of, and Holy Wrath jumps between
        // raiders standing close, so Zeliek's camp spreads twice as wide.
        uint32 seat = 0;
        for (auto const& [guid, assignment] : plan->assignments)
            if (assignment.camp == mine->camp && guid < bot->GetGUID())
                ++seat;

        float const spacing = (mine->camp == 3) ? 12.0f : 6.0f;
        float const angle = float(seat % 6) * (float(M_PI) / 3.0f);

        if (moveTo2d(camp.x + spacing * std::cos(angle), camp.y + spacing * std::sin(angle), 4.0f))
            return true;
    }

    if (mine->duty == RAID_DUTY_HEAL)
        return false;  // in position; the heal engine owns the rest

    // Opening phase: hold the camp, but nothing touches a boss until the
    // tanks have all four parked and threatened.
    if (plan->phase == 1)
    {
        commandPets(nullptr);
        return false;
    }

    // Zeliek's camp is ranged only, and pets are melee.
    commandPets(mine->camp == 3 ? nullptr : boss);

    if (!boss || AI_VALUE(Unit*, "current target") == boss)
        return false;

    return Attack(boss);
}

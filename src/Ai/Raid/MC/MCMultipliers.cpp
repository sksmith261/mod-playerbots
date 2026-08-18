#include "MCMultipliers.h"

#include "Playerbots.h"
#include "ChooseTargetActions.h"
#include "GenericActions.h"
#include "GenericSpellActions.h"
#include "DruidActions.h"
#include "HunterActions.h"
#include "PaladinActions.h"
#include "ShamanActions.h"
#include "WarriorActions.h"
#include "DKActions.h"
#include "MCActions.h"
#include "MCHelpers.h"
#include "RaidBossScripts.h"

using namespace MoltenCoreHelpers;

static bool IsDpsBotWithAoeAction(Player* bot, Action* action)
{
    if (PlayerbotAI::IsDps(bot))
    {
        if (dynamic_cast<DpsAoeAction*>(action) || dynamic_cast<CastConsecrationAction*>(action) ||
            dynamic_cast<CastStarfallAction*>(action) || dynamic_cast<CastWhirlwindAction*>(action) ||
            dynamic_cast<CastMagmaTotemAction*>(action) || dynamic_cast<CastExplosiveTrapAction*>(action) ||
            dynamic_cast<CastDeathAndDecayAction*>(action))
            return true;

        if (auto castSpellAction = dynamic_cast<CastSpellAction*>(action))
        {
            if (castSpellAction->getThreatType() == Action::ActionThreatType::Aoe)
                return true;
        }
    }
    return false;
}

float GarrDisableDpsAoeMultiplier::GetValue(Action* action)
{
    if (AI_VALUE2(Unit*, "find target", "garr"))
    {
        if (IsDpsBotWithAoeAction(bot, action))
            return 0.0f;
    }
    return 1.0f;
}

static bool IsAllowedGeddonMovementAction(Action* action)
{
    // RaidMoveFromGroundEffectAction must stay allowed: a bot with Living
    // Bomb standing in a lingering fire patch needs to flee the patch, and
    // zeroing it left bots taking the DoT for the aura's whole duration.
    if (dynamic_cast<MovementAction*>(action) &&
                !dynamic_cast<McMoveFromGroupAction*>(action) &&
                !dynamic_cast<McMoveFromBaronGeddonAction*>(action) &&
                !dynamic_cast<RaidMoveFromGroundEffectAction*>(action))
        return false;

    if (dynamic_cast<CastReachTargetSpellAction*>(action))
        return false;

    return true;
}

float BaronGeddonAbilityMultiplier::GetValue(Action* action)
{
    if (Unit* boss = AI_VALUE2(Unit*, "find target", "baron geddon"))
    {
        if (boss->HasAura(SPELL_INFERNO))
        {
            if (!IsAllowedGeddonMovementAction(action))
                return 0.0f;
        }
    }

    // No check for Baron Geddon, because bots may have the bomb even after Geddon died.
    if (bot->HasAura(SPELL_LIVING_BOMB))
    {
        if (!IsAllowedGeddonMovementAction(action))
            return 0.0f;
    }

    return 1.0f;
}

float MajordomoReflectionMultiplier::GetValue(Action* action)
{
    // Cheap gates first: the reflection auras exist only on Majordomo's
    // adds, so the current target's auras fully identify the situation — no
    // need for the find-target name scan on every action of every tick.
    Unit* target = AI_VALUE(Unit*, "current target");
    if (!target)
        return 1.0f;

    bool const magicReflection = target->HasAura(SPELL_DOMO_MAGIC_REFLECTION);
    bool const damageReflection = target->HasAura(SPELL_DOMO_DAMAGE_REFLECTION);
    if (!magicReflection && !damageReflection)
        return 1.0f;

    // Only suppress actions actually aimed at the shielded unit: friendly
    // utility (battle-res, decurse, self-heals, buffs) must stay available,
    // whatever the bot's role.
    if (action->GetTarget() != target)
        return 1.0f;

    // Classify by what the ACTION does, not what the bot is: melee classes
    // cast magic (Exorcism, shocks) and casters can melee. Melee specials
    // are CastMeleeSpellAction, not MeleeAction — both are physical.
    bool const isMeleeAttack = dynamic_cast<MeleeAction*>(action) || dynamic_cast<CastMeleeSpellAction*>(action);
    if (damageReflection && isMeleeAttack)
        return 0.0f;

    if (magicReflection && !isMeleeAttack && dynamic_cast<CastSpellAction*>(action))
        return 0.0f;

    return 1.0f;
}

static bool IsSingleLivingTankInGroup(Player* bot)
{
    if (Group* group = bot->GetGroup())
    {
        for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
        {
            Player* member = itr->GetSource();
            if (!member || !member->IsAlive() || member == bot)
                continue;
            if (PlayerbotAI::IsTank(member))
                return false;
        }
    }
    return true;
}

float GolemaggMultiplier::GetValue(Action* action)
{
    if (AI_VALUE2(Unit*, "find target", "golemagg the incinerator"))
    {
        if (PlayerbotAI::IsTank(bot) && IsSingleLivingTankInGroup(bot))
        {
            // Only one tank => Pick up Golemagg and the two Core Ragers
            if (dynamic_cast<McGolemaggMainTankAttackGolemaggAction*>(action) ||
                dynamic_cast<McGolemaggAssistTankAttackCoreRagerAction*>(action))
                return 0.0f;
        }
        if (PlayerbotAI::IsAssistTank(bot))
        {
            // The first two assist tanks manage the Core Ragers. The remaining assist tanks attack the boss.
            if (dynamic_cast<TankAssistAction*>(action))
                return 0.0f;
        }
        if (IsDpsBotWithAoeAction(bot, action))
            return 0.0f;
    }
    return 1.0f;
}

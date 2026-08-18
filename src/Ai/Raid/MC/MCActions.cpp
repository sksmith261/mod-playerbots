#include "MCActions.h"

#include <algorithm>

#include "Playerbots.h"
#include "RtiTargetValue.h"
#include "MCHelpers.h"

static constexpr float LIVING_BOMB_DISTANCE = 20.0f;
static constexpr float INFERNO_DISTANCE = 20.0f;

// don't get hit by Arcane Explosion but still be in casting range
static constexpr float ARCANE_EXPLOSION_DISTANCE = 26.0f;

// dedicated tank positions; prevents assist tanks from positioning Core Ragers on steep walls on pull
static const Position GOLEMAGG_TANK_POSITION{795.7308, -994.8848, -207.18661};
static const Position CORE_RAGER_TANK_POSITION{846.6453, -1019.0639, -198.9819};

static constexpr float GOLEMAGGS_TRUST_DISTANCE = 30.0f;
static constexpr float CORE_RAGER_STEP_DISTANCE = 5.0f;

using namespace MoltenCoreHelpers;

bool McMoveFromGroupAction::Execute(Event /*event*/)
{
    return MoveFromGroup(LIVING_BOMB_DISTANCE);
}

bool McMoveFromBaronGeddonAction::Execute(Event /*event*/)
{
    if (Unit* boss = AI_VALUE2(Unit*, "find target", "baron geddon"))
    {
        float distToTravel = INFERNO_DISTANCE - bot->GetDistance2d(boss);
        if (distToTravel > 0)
        {
            // Stop current spell first
            bot->AttackStop();
            bot->InterruptNonMeleeSpells(false);

            return MoveAway(boss, distToTravel);
        }
    }
    return false;
}

bool McShazzrahMoveAwayAction::Execute(Event /*event*/)
{
    if (Unit* boss = AI_VALUE2(Unit*, "find target", "shazzrah"))
    {
        float distToTravel = ARCANE_EXPLOSION_DISTANCE - bot->GetDistance2d(boss);
        if (distToTravel > 0)
            return MoveAway(boss, distToTravel);
    }
    return false;
}

bool McShazzrahPurgeAction::Execute(Event /*event*/)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "shazzrah");
    if (!boss || !boss->HasAura(SPELL_DEADEN_MAGIC))
        return false;

    if (bot->getClass() == CLASS_SHAMAN)
        return botAI->CastSpell("purge", boss);

    return botAI->CastSpell("dispel magic", boss);
}

bool McGolemaggMarkBossAction::Execute(Event /*event*/)
{
    if (Unit* boss = AI_VALUE2(Unit*, "find target", "golemagg the incinerator"))
    {
        if (Group* group = bot->GetGroup())
        {
            ObjectGuid currentSkullGuid = group->GetTargetIcon(RtiTargetValue::skullIndex);
            if (currentSkullGuid.IsEmpty() || currentSkullGuid != boss->GetGUID())
            {
                group->SetTargetIcon(RtiTargetValue::skullIndex, bot->GetGUID(), boss->GetGUID());
                return true;
            }
        }
    }
    return false;
}

bool McGolemaggTankAction::MoveUnitToPosition(Unit* target, const Position& tankPosition, float maxDistance,
                                              float stepDistance)
{
    if (bot->GetVictim() != target)
        return Attack(target);
    if (target->GetVictim() == bot)
    {
        float distanceToTankPosition = bot->GetExactDist2d(tankPosition.GetPositionX(), tankPosition.GetPositionY());
        if (distanceToTankPosition > maxDistance)
        {
            float dX = tankPosition.GetPositionX() - bot->GetPositionX();
            float dY = tankPosition.GetPositionY() - bot->GetPositionY();
            float dist = sqrt(dX * dX + dY * dY);
            float moveX = bot->GetPositionX() + (dX / dist) * stepDistance;
            float moveY = bot->GetPositionY() + (dY / dist) * stepDistance;
            return MoveTo(bot->GetMapId(), moveX, moveY, bot->GetPositionZ(), false, false,
                          false, false, MovementPriority::MOVEMENT_COMBAT, true,
                          true);
        }
    }
    else if (botAI->DoSpecificAction("taunt spell", Event(), true))
        return true;
    return false;
}

bool McGolemaggTankAction::FindCoreRagers(Unit*& coreRager1, Unit*& coreRager2) const
{
    coreRager1 = coreRager2 = nullptr;
    for (auto const& target : AI_VALUE(GuidVector, "possible targets no los"))
    {
        Unit* unit = botAI->GetUnit(target);
        if (unit && unit->IsAlive() && unit->GetEntry() == NPC_CORE_RAGER)
        {
            if (coreRager1 == nullptr)
                coreRager1 = unit;
            else if (coreRager2 == nullptr)
            {
                coreRager2 = unit;
                break; // There should be no third Core Rager.
            }
        }
    }
    return coreRager1 != nullptr && coreRager2 != nullptr;
}

bool McGolemaggMainTankAttackGolemaggAction::Execute(Event /*event*/)
{
    // At this point, we know we are not the last living tank in the group.
    if (Unit* boss = AI_VALUE2(Unit*, "find target", "golemagg the incinerator"))
    {
        Unit* coreRager1;
        Unit* coreRager2;
        if (!FindCoreRagers(coreRager1, coreRager2))
            return false; // safety check

        // We only need to move if the Core Ragers still have Golemagg's Trust
        if (coreRager1->HasAura(SPELL_GOLEMAGGS_TRUST) || coreRager2->HasAura(SPELL_GOLEMAGGS_TRUST))
            return MoveUnitToPosition(boss, GOLEMAGG_TANK_POSITION, boss->GetCombatReach());
    }
    return false;
}

bool McGolemaggAssistTankAttackCoreRagerAction::Execute(Event event)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "golemagg the incinerator");
    if (!boss)
        return false;

    // Step 0: Filter additional assist tanks. We only need 2.
    bool isFirstAssistTank = PlayerbotAI::IsAssistTankOfIndex(bot, 0, true);
    bool isSecondAssistTank = PlayerbotAI::IsAssistTankOfIndex(bot, 1, true);
    if (!isFirstAssistTank && !isSecondAssistTank)
        return Attack(boss);

    // Step 1: Find both Core Ragers
    Unit* coreRager1;
    Unit* coreRager2;
    if (!FindCoreRagers(coreRager1, coreRager2))
        return false; // safety check

    // Step 2: Assign Core Rager to bot
    Unit* myCoreRager = nullptr;
    Unit* otherCoreRager = nullptr;
    if (isFirstAssistTank)
    {
        myCoreRager = coreRager1;
        otherCoreRager = coreRager2;
    }
    else // isSecondAssistTank is always true here
    {
        myCoreRager = coreRager2;
        otherCoreRager = coreRager1;
    }

    // Step 3: Select the right target
    if (myCoreRager->GetVictim() != bot)
    {
        // Step 3.1: My Core Rager isn't attacking me. Attack until it does.
        if (bot->GetVictim() != myCoreRager)
            return Attack(myCoreRager);
        return botAI->DoSpecificAction("taunt spell", event, true);
    }

    Unit* otherCoreRagerVictim = otherCoreRager->GetVictim();
    if (otherCoreRagerVictim) // Core Rager victim can be NULL
    {
        // Step 3.2: Check if the other Core Rager isn't attacking its assist tank.
        Player* otherCoreRagerPlayerVictim = otherCoreRagerVictim->ToPlayer();
        if (otherCoreRagerPlayerVictim &&
            !PlayerbotAI::IsAssistTankOfIndex(otherCoreRagerPlayerVictim, 0, true) &&
            !PlayerbotAI::IsAssistTankOfIndex(otherCoreRagerPlayerVictim, 1, true))
        {
            // Assume we are the only assist tank or the other assist tank is dead => pick up other Core Rager!
            if (bot->GetVictim() != otherCoreRager)
                return Attack(otherCoreRager);
            return botAI->DoSpecificAction("taunt spell", event, true);
        }
    }

    if (bot->GetVictim() != myCoreRager)
        return Attack(myCoreRager); // Step 3.3: Attack our Core Rager in case we previously switched in 3.2.

    // Step 4: Prevent Golemagg's Trust on Core Ragers
    if (myCoreRager->HasAura(SPELL_GOLEMAGGS_TRUST) ||
        (otherCoreRagerVictim == bot && otherCoreRager->HasAura(SPELL_GOLEMAGGS_TRUST)))
    {
        // Step 4.1: Move Core Ragers to dedicated tank position (only if Golemagg is far enough away from said position)
        float bossDistanceToCoreRagerTankPosition = boss->GetExactDist2d(
            CORE_RAGER_TANK_POSITION.GetPositionX(), CORE_RAGER_TANK_POSITION.GetPositionY());
        if (bossDistanceToCoreRagerTankPosition > GOLEMAGGS_TRUST_DISTANCE)
        {
            float distanceToTankPosition = bot->GetExactDist2d(CORE_RAGER_TANK_POSITION.GetPositionX(),
                                                               CORE_RAGER_TANK_POSITION.GetPositionY());
            if (distanceToTankPosition > CORE_RAGER_STEP_DISTANCE)
                return MoveUnitToPosition(myCoreRager, CORE_RAGER_TANK_POSITION, CORE_RAGER_STEP_DISTANCE);
        }

        // Step 4.2: if boss is too close to tank position, or we are already there, move away from Golemagg to try to out-range Golemagg's Trust
        return MoveAway(boss, CORE_RAGER_STEP_DISTANCE, true);
    }

    return false;
}

std::vector<Unit*> MoltenCoreHelpers::GetLivingFiresworn(PlayerbotAI* botAI)
{
    std::vector<Unit*> firesworn;
    for (auto const& target : botAI->GetAiObjectContext()->GetValue<GuidVector>("possible targets no los")->Get())
    {
        Unit* unit = botAI->GetUnit(target);
        if (unit && unit->IsAlive() && unit->GetEntry() == NPC_FIRESWORN)
            firesworn.push_back(unit);
    }

    std::sort(firesworn.begin(), firesworn.end(), [](Unit* a, Unit* b)
              { return a->GetGUID().GetCounter() < b->GetGUID().GetCounter(); });
    return firesworn;
}

bool MoltenCoreHelpers::IsBanished(Unit* unit)
{
    return unit->HasAura(SPELL_BANISH_R1) || unit->HasAura(SPELL_BANISH_R2);
}

uint32 MoltenCoreHelpers::CountGarrBanishAssignments(PlayerbotAI* botAI, Player* bot)
{
    uint32 warlocks = 0;
    if (Group* group = bot->GetGroup())
        for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
        {
            Player* member = itr->GetSource();
            if (member && member->IsAlive() && member->getClass() == CLASS_WARLOCK)
                ++warlocks;
        }

    return std::min<uint32>(warlocks, 2);
}

Unit* MoltenCoreHelpers::GetGarrBanishAssignment(PlayerbotAI* botAI, Player* bot)
{
    if (bot->getClass() != CLASS_WARLOCK)
        return nullptr;

    // My rank among the group's living warlocks, in shared iteration order.
    int32 myRank = -1;
    uint32 count = 0;
    if (Group* group = bot->GetGroup())
        for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
        {
            Player* member = itr->GetSource();
            if (!member || !member->IsAlive() || member->getClass() != CLASS_WARLOCK)
                continue;

            if (member == bot)
                myRank = count;

            ++count;
        }

    if (myRank < 0 || myRank > 1)
        return nullptr;

    std::vector<Unit*> firesworn = GetLivingFiresworn(botAI);
    if (static_cast<uint32>(myRank) >= firesworn.size())
        return nullptr;

    return firesworn[myRank];
}

bool McGarrBanishAction::Execute(Event /*event*/)
{
    Unit* target = MoltenCoreHelpers::GetGarrBanishAssignment(botAI, bot);
    if (!target || MoltenCoreHelpers::IsBanished(target) || target->HasAura(SPELL_SEPARATION_ANXIETY_MINION))
        return false;

    return botAI->CastSpell("banish", target);
}

Unit* McGarrMarkAction::GetTarget()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "garr");
    if (!boss)
        return nullptr;

    Group* group = bot->GetGroup();
    if (!group)
        return nullptr;

    std::vector<Unit*> firesworn = MoltenCoreHelpers::GetLivingFiresworn(botAI);
    uint32 const reserved = MoltenCoreHelpers::CountGarrBanishAssignments(botAI, bot);

    ObjectGuid currentSkullGuid = group->GetTargetIcon(RtiTargetValue::skullIndex);
    Unit* currentSkullUnit = currentSkullGuid.IsEmpty() ? nullptr : botAI->GetUnit(currentSkullGuid);

    Unit* best = nullptr;
    for (uint32 i = 0; i < firesworn.size(); ++i)
    {
        // The first `reserved` GUID-sorted adds belong to the banishing
        // warlocks — never mark them, even before the banish lands.
        if (i < reserved)
            continue;

        Unit* unit = firesworn[i];
        if (MoltenCoreHelpers::IsBanished(unit))
            continue;

        // Keep the current skull while it is still a valid kill target, so
        // the mark doesn't flap between equally-damaged adds.
        if (unit == currentSkullUnit)
            return nullptr;

        if (!best || unit->GetHealth() < best->GetHealth())
            best = unit;
    }

    if (best)
        return best;

    // Only banished adds (and Garr) remain: kill Garr with the pair tucked
    // away, classic style.
    if (currentSkullGuid.IsEmpty() || currentSkullGuid != boss->GetGUID())
        return boss;

    return nullptr;
}

bool McGarrMarkAction::Execute(Event /*event*/)
{
    Unit* target = GetTarget();
    if (!target)
        return false;

    bot->GetGroup()->SetTargetIcon(RtiTargetValue::skullIndex, bot->GetGUID(), target->GetGUID());
    return true;
}

bool McMagmadarFearWardAction::Execute(Event /*event*/)
{
    Group* group = bot->GetGroup();
    if (!group)
        return false;

    for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
    {
        Player* member = itr->GetSource();
        if (member && member->IsAlive() && botAI->IsMainTank(member))
        {
            if (member->HasAura(SPELL_FEAR_WARD))
                return false;

            return botAI->CastSpell(SPELL_FEAR_WARD, member);
        }
    }

    return false;
}

bool McMagmadarMoveFromLavaAction::Execute(Event /*event*/)
{
    // Any direction out of the patch works; FleePosition picks a safe nearby
    // spot away from where the bot is standing.
    return FleePosition(bot->GetPosition(), 8.0f);
}

Unit* McKillOrderMarkAction::GetTarget()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", bossName);
    if (!boss)
        return nullptr;

    Group* group = bot->GetGroup();
    if (!group)
        return nullptr;

    ObjectGuid currentSkullGuid = group->GetTargetIcon(RtiTargetValue::skullIndex);
    Unit* currentSkullUnit = currentSkullGuid.IsEmpty() ? nullptr : botAI->GetUnit(currentSkullGuid);

    for (uint32 entry : addEntries)
    {
        Unit* best = nullptr;
        bool bestClean = false;
        for (auto const& target : AI_VALUE(GuidVector, "possible targets no los"))
        {
            Unit* unit = botAI->GetUnit(target);
            if (!unit || !unit->IsAlive() || unit->GetEntry() != entry)
                continue;

            // Prefer targets without a reflection shield; the most damaged
            // otherwise, so an in-progress kill finishes.
            bool const clean = !avoidReflections || (!unit->HasAura(SPELL_DOMO_MAGIC_REFLECTION) &&
                                                    !unit->HasAura(SPELL_DOMO_DAMAGE_REFLECTION));
            if (!best || (clean && !bestClean) ||
                (clean == bestClean && unit->GetHealth() < best->GetHealth()))
            {
                best = unit;
                bestClean = clean;
            }
        }

        if (!best)
            continue;  // tier cleared; next tier

        // Sticky within the active tier so the mark doesn't flap between
        // equally-valid adds — but with avoidReflections, switch off a target
        // that picked up a shield while a clean one exists.
        if (currentSkullUnit && currentSkullUnit->IsAlive() && currentSkullUnit->GetEntry() == entry)
        {
            bool const currentClean =
                !avoidReflections || (!currentSkullUnit->HasAura(SPELL_DOMO_MAGIC_REFLECTION) &&
                                      !currentSkullUnit->HasAura(SPELL_DOMO_DAMAGE_REFLECTION));
            if (currentClean || !bestClean)
                return nullptr;
        }

        return best;
    }

    // All add tiers cleared: skull the boss.
    if (currentSkullGuid.IsEmpty() || currentSkullGuid != boss->GetGUID())
        return boss;

    return nullptr;
}

bool McKillOrderMarkAction::Execute(Event /*event*/)
{
    Unit* target = GetTarget();
    if (!target)
        return false;

    bot->GetGroup()->SetTargetIcon(RtiTargetValue::skullIndex, bot->GetGUID(), target->GetGUID());
    return true;
}

Unit* McCoreHoundMarkAction::GetTarget()
{
    Unit* highestHealthHound = nullptr;
    for (auto const& [guid, ref] : bot->GetThreatMgr().GetThreatenedByMeList())
    {
        Unit* unit = ref->GetOwner();
        if (unit && unit->IsAlive() && unit->GetEntry() == NPC_CORE_HOUND)
        {
            if (!highestHealthHound || unit->GetHealth() > highestHealthHound->GetHealth())
                highestHealthHound = unit;
        }
    }

    if (!highestHealthHound)
        return nullptr;

    Group* group = bot->GetGroup();
    ObjectGuid currentSkullGuid = group ? group->GetTargetIcon(RtiTargetValue::skullIndex) : ObjectGuid::Empty;
    if (!currentSkullGuid.IsEmpty() && currentSkullGuid != highestHealthHound->GetGUID())
    {
        // Only switch skull if the new target has meaningfully more health (10% buffer) to prevent rapid re-marking
        if (Unit* currentSkullUnit = botAI->GetUnit(currentSkullGuid))
            if (currentSkullUnit->IsAlive() && highestHealthHound->GetHealth() <= currentSkullUnit->GetHealth() * 1.10f)
                return nullptr;
    }

    if (currentSkullGuid.IsEmpty() || currentSkullGuid != highestHealthHound->GetGUID())
        return highestHealthHound;

    return nullptr;
}

bool McCoreHoundMarkAction::Execute(Event /*event*/)
{
    Unit* target = GetTarget();
    if (!target)
        return false;

    bot->GetGroup()->SetTargetIcon(RtiTargetValue::skullIndex, bot->GetGUID(), target->GetGUID());
    return true;
}

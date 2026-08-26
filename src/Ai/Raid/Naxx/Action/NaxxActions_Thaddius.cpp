#include "NaxxActions.h"

#include "PlayerbotAIConfig.h"
#include "Playerbots.h"
#include "NaxxSpellIds.h"

bool ThaddiusAttackNearestPetAction::isUseful()
{
    if (!helper.UpdateBossAI())
        return false;

    if (!helper.IsPhasePet())
        return false;

    Unit* target = helper.GetAssignedPet(bot);
    if (!target)
        return false;

    return true;
}

bool ThaddiusAttackNearestPetAction::Execute(Event /*event*/)
{
    // Tanks follow the pet they currently hold (Magnetic Pull reassigns
    // them every 20s); everyone else keeps their static assignment, which
    // is what keeps the raid evenly split from the pull onward.
    Unit* target = PlayerbotAI::IsTank(bot) ? helper.GetTankPet(bot) : helper.GetAssignedPet(bot);
    if (!target)
        return false;

    // Healers never receive an attack order here — being told to Attack
    // the pet made the approach movement fight their ranged positioning
    // (the observed wandering). They take their platform spot and let the
    // heal engine work.
    if (botAI->IsHeal(bot))
    {
        std::pair<float, float> posForRanged = helper.PetPhaseGetPosForRanged();
        if (bot->GetExactDist2d(posForRanged.first, posForRanged.second) < 6.0f)
            return false;
        return MoveTo(533, posForRanged.first, posForRanged.second, helper.tankPosZ, false, false, false, false,
                      MovementPriority::MOVEMENT_COMBAT);
    }

    // Until the pet has walked down off its platform, every path to it
    // fails (no mesh at platform height) and chasing it thrashes at the
    // base. Hold the verified floor camp; the pet aggros and comes to us —
    // which is also how the fight was tanked before any automation.
    if (target->GetPositionZ() > 306.0f)
    {
        std::pair<float, float> camp =
            botAI->IsRanged(bot) ? helper.PetPhaseGetPosForRanged() : helper.PetPhaseGetPosForTank();
        return MoveInside(533, camp.first, camp.second, helper.tankPosZ, 5.0f,
                          MovementPriority::MOVEMENT_COMBAT);
    }

    if (!bot->IsWithinLOSInMap(target))
        return MoveTo(target, 0, MovementPriority::MOVEMENT_COMBAT);

    if (AI_VALUE(Unit*, "current target") != target)
        return Attack(target);

    if (botAI->IsTank(bot) && AI_VALUE2(bool, "has aggro", "current target"))
    {
        std::pair<float, float> posForTank = helper.PetPhaseGetPosForTank();
        return MoveTo(533, posForTank.first, posForTank.second, helper.tankPosZ, false, false, false, false, MovementPriority::MOVEMENT_COMBAT);
    }
    if (botAI->IsRanged(bot))
    {
        std::pair<float, float> posForRanged = helper.PetPhaseGetPosForRanged();
        return MoveTo(533, posForRanged.first, posForRanged.second, helper.tankPosZ, false, false, false, false, MovementPriority::MOVEMENT_COMBAT);
    }
    return false;
}

bool ThaddiusMoveToPlatformAction::isUseful() { return true; }

bool ThaddiusMoveToPlatformAction::Execute(Event /*event*/)
{
    // Ledge (jump-off) and landing points per side, then the platform centre.
    std::vector<std::pair<float, float>> position = {
        // high left (Stalagg-side ledge)
        {3462.99f, -2918.90f},
        // high right (Feugen-side ledge)
        {3520.65f, -2976.51f},
        // low left (landing)
        {3471.36f, -2910.65f},
        // low right (landing)
        {3528.80f, -2967.04f},
        // centre of Thaddius's platform
        {3512.19f, -2928.58f},
    };
    float const high_z = 312.00f, low_z = 304.02f;

    bool const is_left = bot->GetDistance2d(position[0].first, position[0].second) <
                         bot->GetDistance2d(position[1].first, position[1].second);

    // Still up on a pet platform: walk to this side's ledge, then leap.
    if (bot->GetPositionZ() >= (high_z - 3.0f))
    {
        std::pair<float, float> const& ledge = is_left ? position[0] : position[1];
        std::pair<float, float> const& landing = is_left ? position[2] : position[3];

        // Upstream only jumped when a MoveTo had FAILED and the bot was
        // within contactDistance (0.45y) of the exact ledge coordinate —
        // a tolerance pathfinding essentially never lands on, so bots
        // walked to the ledge and stood there for the rest of the fight.
        // Distance decides now, and 4y is a tolerance walking can hit.
        if (bot->GetExactDist2d(ledge.first, ledge.second) < 4.0f)
        {
            if (JumpTo(bot->GetMapId(), landing.first, landing.second, low_z,
                       MovementPriority::MOVEMENT_COMBAT))
                return true;

            return false;  // jump refused this tick (spline busy) — retry
        }

        return MoveTo(bot->GetMapId(), ledge.first, ledge.second, high_z, false, false, false,
                      /*exact_waypoint*/ true, MovementPriority::MOVEMENT_COMBAT);
    }

    // Landed: regroup at the centre; polarity takes over from there.
    return MoveTo(bot->GetMapId(), position[4].first, position[4].second, low_z, false, false, false, false,
                  MovementPriority::MOVEMENT_COMBAT);
}

bool ThaddiusMovePolarityAction::isUseful()
{
    return !botAI->IsMainTank(bot) || AI_VALUE2(bool, "has aggro", "current target");
}

bool ThaddiusMovePolarityAction::Execute(Event /*event*/)
{
    std::vector<std::pair<float, float>> position = {
        // left melee (negative) — bounded by melee reach on the boss
        {3508.29f, -2920.12f},
        // left ranged (negative) — pushed wide: 40 bodies need separation
        {3499.50f, -2915.90f},
        // right melee (positive)
        {3519.74f, -2931.69f},
        // right ranged (positive)
        {3524.90f, -2941.30f},
    };
    int32 const myCharge = NaxxHelpers::ThaddiusCharge(botAI, bot);
    if (myCharge < 0)
    {
        // Chargeless = the strip gap inside every Polarity Shift (old
        // charges removed an instant before new ones land). Upstream sent
        // everyone sprinting to a CENTER spot here, collapsing both
        // clusters through each other every 30s. Hold instead.
        return false;
    }

    bool const rangedSide = PlayerbotAI::IsRanged(bot) || PlayerbotAI::IsHeal(bot);
    uint32 const mine = uint32(myCharge) * 2 + (rangedSide ? 1u : 0u);
    uint32 const opposite = (myCharge ? 0u : 2u) + (rangedSide ? 1u : 0u);

    // Rank among the bots sharing this cluster, in shared group order, so
    // every bot lands on its own seat instead of all forty being sent to
    // one identical point and shoving each other for it.
    uint32 rank = 0;
    if (Group* group = bot->GetGroup())
        for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
        {
            Player* member = itr->GetSource();
            if (!member || !member->IsAlive() || !GET_PLAYERBOT_AI(member))
                continue;

            if (member == bot)
                break;

            if (NaxxHelpers::ThaddiusCharge(GET_PLAYERBOT_AI(member), member) != myCharge)
                continue;

            if ((PlayerbotAI::IsRanged(member) || PlayerbotAI::IsHeal(member)) != rangedSide)
                continue;

            ++rank;
        }

    float x = position[mine].first;
    float y = position[mine].second;

    // Tanks hold the cluster anchor so the boss stays between the two melee
    // camps. Everyone else fans out on an arc facing AWAY from the opposite
    // charge: seats never grow toward the other cluster (10y is both the
    // damage radius and the same-charge stack radius), and melee stay inside
    // Thaddius's 10y reach.
    if (!PlayerbotAI::IsTank(bot))
    {
        float const away = std::atan2(position[mine].second - position[opposite].second,
                                      position[mine].first - position[opposite].first);

        float const base = rangedSide ? 2.4f : 1.6f;
        float const step = rangedSide ? 1.5f : 0.9f;
        float const cap = rangedSide ? 5.0f : 3.0f;
        float const radius = std::min(base + step * float(rank / 6), cap);
        float const angle = away + (float(rank % 6) - 2.5f) * (float(M_PI) / 7.0f);

        x += radius * std::cos(angle);
        y += radius * std::sin(angle);
    }

    float z = bot->GetPositionZ();
    bot->UpdateAllowedPositionZ(x, y, z);

    // Settled: stop re-issuing the move every tick.
    if (bot->GetExactDist2d(x, y) < 2.0f)
        return false;

    return MoveTo(bot->GetMapId(), x, y, z, false, false, false, false, MovementPriority::MOVEMENT_COMBAT);
}

bool ThaddiusTetherAction::Execute(Event /*event*/)
{
    Unit* pet = helper.GetHeldPet(bot);
    if (!pet)
        return false;

    Creature* creature = pet->ToCreature();
    if (!creature)
        return false;

    // Walk back to the pet's spawn; it follows its aggro home with us.
    Position const& home = creature->GetHomePosition();
    float x = home.GetPositionX();
    float y = home.GetPositionY();
    float z = home.GetPositionZ();

    if (bot->GetExactDist2d(x, y) < 7.0f)
        return false;

    bot->UpdateAllowedPositionZ(x, y, z);
    return MoveNear(bot->GetMapId(), x, y, z, 6.0f, MovementPriority::MOVEMENT_COMBAT);
}

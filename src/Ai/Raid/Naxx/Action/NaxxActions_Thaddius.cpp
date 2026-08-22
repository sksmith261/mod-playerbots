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
    std::vector<std::pair<float, float>> position = {
        // high left
        {3462.99f, -2918.90f},
        // high right
        {3520.65f, -2976.51f},
        // low left
        {3471.36f, -2910.65f},
        // low right
        {3528.80f, -2967.04f},
        // center
        {3512.19f, -2928.58f},
    };
    float high_z = 312.00f, low_z = 304.02f;
    bool is_left = bot->GetDistance2d(position[0].first, position[0].second) <
                   bot->GetDistance2d(position[1].first, position[1].second);
    if (bot->GetPositionZ() >= (high_z - 3.0f))
    {
        if (is_left)
        {
            if (!MoveTo(bot->GetMapId(), position[0].first, position[0].second, high_z, false, false, false, false, MovementPriority::MOVEMENT_COMBAT))
            {
                float distance = bot->GetExactDist2d(position[0].first, position[0].second);
                if (distance < sPlayerbotAIConfig.contactDistance)
                    JumpTo(bot->GetMapId(), position[2].first, position[2].second, low_z, MovementPriority::MOVEMENT_COMBAT);
                    // bot->TeleportTo(bot->GetMapId(), position[2].first, position[2].second, low_z, bot->GetOrientation());
            }
        }
        else
        {
            if (!MoveTo(bot->GetMapId(), position[1].first, position[1].second, high_z, false, false, false, false, MovementPriority::MOVEMENT_COMBAT))
            {
                float distance = bot->GetExactDist2d(position[1].first, position[1].second);
                if (distance < sPlayerbotAIConfig.contactDistance)
                    JumpTo(bot->GetMapId(), position[3].first, position[3].second, low_z, MovementPriority::MOVEMENT_COMBAT);
                    // bot->TeleportTo(bot->GetMapId(), position[3].first, position[3].second, low_z, bot->GetOrientation());
            }
        }
    }
    else
        return MoveTo(bot->GetMapId(), position[4].first, position[4].second, low_z, false, false, false, false, MovementPriority::MOVEMENT_COMBAT);

    return true;
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

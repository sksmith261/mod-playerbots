#include "NaxxActions.h"
#include "RaidDirector.h"

#include "PlayerbotAIConfig.h"
#include "Playerbots.h"
#include "NaxxBossHelper.h"
#include "NaxxSpellIds.h"

bool SapphironGroundPositionAction::Execute(Event /*event*/)
{
    if (!helper.UpdateBossAI())
        return false;

    if (botAI->IsMainTank(bot))
    {
        if (AI_VALUE2(bool, "has aggro", "current target"))
            return MoveTo(NAXX_MAP_ID, helper.mainTankPos.first, helper.mainTankPos.second, helper.GENERIC_HEIGHT, false, false, false,
                          false, MovementPriority::MOVEMENT_COMBAT);

        return false;
    }
    if (helper.JustLanded())
    {
        uint32 index = botAI->GetGroupSlotIndex(bot);
        float start_angle = 0.85 * M_PI;
        float offset_angle = M_PI * 0.02 * index;
        float angle = start_angle + offset_angle;
        float distance;
        if (botAI->IsRanged(bot))
            distance = 35.0f;
        else if (botAI->IsHeal(bot))
            distance = 30.0f;
        else
            distance = 5.0f;

        float posX = helper.center.first + cos(angle) * distance;
        float posY = helper.center.second + sin(angle) * distance;
        if (MoveTo(NAXX_MAP_ID, posX, posY, helper.GENERIC_HEIGHT, false, false, false, false, MovementPriority::MOVEMENT_COMBAT))
            return true;

        return MoveInside(NAXX_MAP_ID, posX, posY, helper.GENERIC_HEIGHT, 2.0f, MovementPriority::MOVEMENT_COMBAT);
    }
    else
    {
        std::vector<float> dest;
        if (helper.FindPosToAvoidChill(dest))
            return MoveTo(NAXX_MAP_ID, dest[0], dest[1], dest[2], false, false, false, false, MovementPriority::MOVEMENT_COMBAT);
    }
    return false;
}

bool SapphironFlightPositionAction::Execute(Event /*event*/)
{
    if (!helper.UpdateBossAI())
        return false;

    if (helper.WaitForExplosion())
        return MoveToNearestIcebolt();
    else
    {
        std::vector<float> dest;
        if (helper.FindPosToAvoidChill(dest))
            return MoveTo(NAXX_MAP_ID, dest[0], dest[1], dest[2], false, false, false, false, MovementPriority::MOVEMENT_COMBAT);
    }
    return false;
}

bool SapphironFlightPositionAction::MoveToNearestIcebolt()
{
    Group* group = bot->GetGroup();
    if (!group)
        return false;

    Player* playerWithIcebolt = nullptr;
    float minDistance;
    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (NaxxSpellIds::HasAnyAura(member, {NaxxSpellIds::Icebolt10, NaxxSpellIds::Icebolt25}) ||
            botAI->HasAura("icebolt", member, false, false, -1, true))
        {
            if (!playerWithIcebolt || minDistance > bot->GetDistance(member))
            {
                playerWithIcebolt = member;
                minDistance = bot->GetDistance(member);
            }
        }
    }
    if (playerWithIcebolt)
    {
        Unit* boss = AI_VALUE2(Unit*, "find target", "sapphiron");
        if (boss)
        {
            float angle = boss->GetAngle(playerWithIcebolt);
            float posX = playerWithIcebolt->GetPositionX() + cos(angle) * 3.0f;
            float posY = playerWithIcebolt->GetPositionY() + sin(angle) * 3.0f;
            if (MoveTo(NAXX_MAP_ID, posX, posY, helper.GENERIC_HEIGHT, false, false, false, false, MovementPriority::MOVEMENT_COMBAT))
                return true;

            return MoveNear(playerWithIcebolt, 3.0f, MovementPriority::MOVEMENT_COMBAT);
        }
    }
    return false;
}

// Executes whatever the raid plan assigned. Ground phase is positioning and
// damage; air phase is entirely about putting an ice block between this bot
// and Sapphiron.
bool SapphironPlanAction::Execute(Event /*event*/)
{
    RaidDirector::Tick(bot);
    RaidPlan const* plan = RaidDirector::Get(bot);
    if (!plan || plan->encounter != RAID_ENCOUNTER_SAPPHIRON)
        return false;

    RaidAssignment const* mine = plan->For(bot->GetGUID());
    if (!mine)
        return false;

    constexpr float SAPP_X = 3522.39f;
    constexpr float SAPP_Y = -5236.78f;
    constexpr float SAPP_RING = 26.0f;

    auto moveTo2d = [&](float x, float y, float tolerance) -> bool
    {
        if (bot->GetExactDist2d(x, y) <= tolerance)
            return false;

        float z = bot->GetPositionZ();
        bot->UpdateAllowedPositionZ(x, y, z);
        return MoveInside(bot->GetMapId(), x, y, z, tolerance * 0.75f, MovementPriority::MOVEMENT_COMBAT);
    };

    // ---- Air phase: get behind the assigned block ------------------------
    if (mine->duty == RAID_DUTY_HIDE)
    {
        if (!mine->target)
            return false;  // encased ourselves, or no block exists yet

        GameObject* block = ObjectAccessor::GetGameObject(*bot, mine->target);
        if (!block)
            return false;

        Unit* sapphiron = AI_VALUE2(Unit*, "find target", "sapphiron");

        // Stand on the far side of the block from her, close in, so the
        // block itself is what the breath meets.
        float x = block->GetPositionX();
        float y = block->GetPositionY();
        if (sapphiron)
        {
            float const dx = x - sapphiron->GetPositionX();
            float const dy = y - sapphiron->GetPositionY();
            float const len = std::sqrt(dx * dx + dy * dy);
            if (len > 1.0f)
            {
                x += dx / len * 3.0f;
                y += dy / len * 3.0f;
            }
        }

        botAI->InterruptSpell();  // nothing is worth eating a breath for
        moveTo2d(x, y, 2.0f);
        return true;              // hold the tick: cover outranks everything
    }

    // ---- Ground phase ----------------------------------------------------
    Unit* sapphiron = mine->target ? ObjectAccessor::GetUnit(*bot, mine->target) : nullptr;
    if (!sapphiron || !sapphiron->IsAlive())
        return false;

    if (mine->duty == RAID_DUTY_TANK)
    {
        // Hold her on her spawn rather than dragging her across the room.
        if (moveTo2d(SAPP_X, SAPP_Y, 8.0f))
            return true;

        if (AI_VALUE(Unit*, "current target") != sapphiron)
            return Attack(sapphiron);

        return false;
    }

    if (mine->camp > 0)
    {
        // Ranged and healers ring her. The spread is not only for Frost Aura
        // and Blizzard: the next air phase builds its ice blocks wherever
        // people are standing, so a raid stacked here has no cover later.
        float const angle = float(mine->camp) * 0.45f;
        if (moveTo2d(SAPP_X + SAPP_RING * std::cos(angle), SAPP_Y + SAPP_RING * std::sin(angle), 5.0f))
            return true;
    }

    if (mine->duty == RAID_DUTY_HEAL)
        return false;

    if (AI_VALUE(Unit*, "current target") != sapphiron)
        return Attack(sapphiron);

    return false;
}

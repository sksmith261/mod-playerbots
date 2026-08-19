#include "Aq40Triggers.h"

#include "Aq40Utils.h"
#include "RtiTargetValue.h"

bool Aq40InStomachTrigger::IsActive()
{
    if (bot->GetMapId() != RaidAq40::MAP_TEMPLE_OF_AHNQIRAJ || !bot->IsAlive())
        return false;

    if (bot->GetPositionZ() > RaidAq40::STOMACH_MAX_Z)
        return false;

    return bot->GetDistance2d(RaidAq40::STOMACH_X, RaidAq40::STOMACH_Y) < RaidAq40::STOMACH_RANGE_2D;
}

bool Aq40DarkGlareTrigger::IsActive()
{
    if (bot->GetMapId() != RaidAq40::MAP_TEMPLE_OF_AHNQIRAJ || !bot->IsAlive())
        return false;

    // Bots down in the stomach are handled by the stomach logic.
    if (bot->GetPositionZ() <= RaidAq40::STOMACH_MAX_Z)
        return false;

    Unit* eye = AI_VALUE2(Unit*, "find target", "eye of c'thun");
    if (!eye || !eye->IsAlive() || !eye->HasAura(RaidAq40::SPELL_RED_COLORATION))
        return false;

    // The eye's live orientation is the beam angle; react when it is close
    // to this bot's angular position around the eye.
    float const delta = RaidAq40::AngleDelta(eye->GetAngle(bot), eye->GetOrientation());
    return std::fabs(delta) < RaidAq40::DARK_GLARE_DANGER_ARC;
}

bool Aq40SarturaWhirlwindTrigger::IsActive()
{
    if (bot->GetMapId() != RaidAq40::MAP_TEMPLE_OF_AHNQIRAJ || !bot->IsAlive())
        return false;

    if (!AI_VALUE2(Unit*, "find target", "battleguard sartura"))
        return false;

    for (auto const& guid : AI_VALUE(GuidVector, "attackers"))
    {
        Unit* unit = botAI->GetUnit(guid);
        if (!unit || !unit->IsAlive() || bot->GetDistance(unit) > RaidAq40::WHIRLWIND_DANGER_RANGE)
            continue;

        if (unit->HasAura(RaidAq40::SPELL_SARTURA_WHIRLWIND) || unit->HasAura(RaidAq40::SPELL_GUARD_WHIRLWIND))
            return true;
    }

    return false;
}

bool Aq40TwinsWrongTargetTrigger::IsActive()
{
    if (bot->GetMapId() != RaidAq40::MAP_TEMPLE_OF_AHNQIRAJ || !bot->IsAlive())
        return false;

    // Tanks and healers manage their own targets.
    if (PlayerbotAI::IsTank(bot) || PlayerbotAI::IsHeal(bot))
        return false;

    Unit* veklor = AI_VALUE2(Unit*, "find target", "emperor vek'lor");
    Unit* veknilash = AI_VALUE2(Unit*, "find target", "emperor vek'nilash");
    Unit* current = AI_VALUE(Unit*, "current target");
    if (!current)
        return false;

    // Only correct bots aimed at the twin they cannot hurt; adds and bugs
    // are legitimate targets.
    if (RaidAq40::IsCasterDps(bot))
        return veklor && current == veknilash;

    return veknilash && current == veklor;
}

bool Aq40TwinsTankPickupTrigger::IsActive()
{
    if (bot->GetMapId() != RaidAq40::MAP_TEMPLE_OF_AHNQIRAJ || !bot->IsAlive() || !PlayerbotAI::IsTank(bot))
        return false;

    // Tanks hold VEK'NILASH ONLY. Vek'lor is immune to physical damage: a
    // plate tank can never build threat on him (classic warlock-tanked him),
    // so sending one just parks it in Arcane Burst range doing nothing.
    // Vek'lor is held by the caster team's damage threat — his victim gets
    // dragged apart and healed by the separation logic.
    Unit* veknilash = AI_VALUE2(Unit*, "find target", "emperor vek'nilash");
    if (!veknilash || !veknilash->IsAlive() || !veknilash->IsInCombat())
        return false;

    return AI_VALUE(Unit*, "current target") != veknilash;
}

bool Aq40TwinsSeparateTrigger::IsActive()
{
    if (bot->GetMapId() != RaidAq40::MAP_TEMPLE_OF_AHNQIRAJ || !bot->IsAlive())
        return false;

    Unit* veklor = AI_VALUE2(Unit*, "find target", "emperor vek'lor");
    Unit* veknilash = AI_VALUE2(Unit*, "find target", "emperor vek'nilash");
    if (!veklor || !veknilash || veklor->GetVictim() != bot)
        return false;

    return veklor->GetDistance(veknilash) < RaidAq40::TWINS_SEPARATION_RANGE;
}

bool Aq40TwinsCasterRangeTrigger::IsActive()
{
    if (bot->GetMapId() != RaidAq40::MAP_TEMPLE_OF_AHNQIRAJ || !bot->IsAlive() || !RaidAq40::IsCasterDps(bot))
        return false;

    Unit* veklor = AI_VALUE2(Unit*, "find target", "emperor vek'lor");
    if (!veklor || AI_VALUE(Unit*, "current target") != veklor)
        return false;

    // Do not kite him while he is chasing us; the separate action owns that.
    if (veklor->GetVictim() == bot)
        return false;

    return bot->GetDistance(veklor) < RaidAq40::TWINS_CASTER_MIN_RANGE;
}

bool Aq40OuroMoundTrigger::IsActive()
{
    if (bot->GetMapId() != RaidAq40::MAP_TEMPLE_OF_AHNQIRAJ || !bot->IsAlive())
        return false;

    Creature* mound = bot->FindNearestCreature(RaidAq40::NPC_OURO_DIRT_MOUND, RaidAq40::OURO_MOUND_FLEE_RANGE);
    return mound && mound->IsAlive() && mound->IsInCombat() && mound->GetVictim() == bot;
}

bool Aq40GiantClawSitterTrigger::IsActive()
{
    if (bot->GetMapId() != RaidAq40::MAP_TEMPLE_OF_AHNQIRAJ || !bot->IsAlive() || !PlayerbotAI::IsMelee(bot))
        return false;

    if (!IsRaidGroupInCombat(bot))
        return false;

    Creature* tentacle = bot->FindNearestCreature(RaidAq40::NPC_GIANT_CLAW_TENTACLE, 80.0f);
    if (!tentacle || !tentacle->IsAlive() || !tentacle->IsInCombat())
        return false;

    // Elected sitter: first living melee bot in shared group order — every
    // bot computes the same answer, so exactly one walks over.
    if (Group* group = bot->GetGroup())
        for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
        {
            Player* member = itr->GetSource();
            if (!member || !member->IsAlive() || !GET_PLAYERBOT_AI(member) || !PlayerbotAI::IsMelee(member))
                continue;

            if (member != bot)
                return false;

            return bot->GetDistance(tentacle) > RaidAq40::GIANT_CLAW_SIT_RANGE;
        }

    return false;
}

// Shared by the Skeram pickup trigger and action so both compute the same
// assignment: GUID-sorted living Skerams whose victim is not a tank, and
// this bot's rank among the group's taunt-capable bot tanks.
Unit* RaidAq40::GetSkeramPickupAssignment(PlayerbotAI* botAI, Player* bot)
{
    if (!PlayerbotAI::IsTank(bot) || !bot->IsAlive())
        return nullptr;

    std::list<Creature*> skerams;
    bot->GetCreatureListWithEntryInGrid(skerams, RaidAq40::NPC_PROPHET_SKERAM, 120.0f);

    std::vector<Unit*> untanked;
    for (Creature* skeram : skerams)
    {
        // Grid-found, not threat-found: an unengaged Skeram has no victim
        // and read as "untanked", sending tanks to body-pull him from the
        // trash. Only fight-engaged Skerams need pickup.
        if (!skeram->IsAlive() || !skeram->IsInCombat())
            continue;

        Unit* victim = skeram->GetVictim();
        Player* victimPlayer = victim ? victim->ToPlayer() : nullptr;
        if (victimPlayer && PlayerbotAI::IsTank(victimPlayer))
            continue;

        untanked.push_back(skeram);
    }

    if (untanked.empty())
        return nullptr;

    std::sort(untanked.begin(), untanked.end(), [](Unit* a, Unit* b)
              { return a->GetGUID().GetCounter() < b->GetGUID().GetCounter(); });

    int32 myRank = -1;
    uint32 count = 0;
    if (Group* group = bot->GetGroup())
        for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
        {
            Player* member = itr->GetSource();
            if (!member || !member->IsAlive() || !PlayerbotAI::IsTank(member) || !GET_PLAYERBOT_AI(member))
                continue;

            if (member == bot)
                myRank = count;

            ++count;
        }

    if (myRank < 0 || static_cast<uint32>(myRank) >= untanked.size())
        return nullptr;

    return untanked[myRank];
}

bool Aq40SkeramTankPickupTrigger::IsActive()
{
    if (bot->GetMapId() != RaidAq40::MAP_TEMPLE_OF_AHNQIRAJ)
        return false;

    if (!IsRaidGroupInCombat(bot))
        return false;

    Unit* assigned = RaidAq40::GetSkeramPickupAssignment(botAI, bot);
    return assigned && bot->GetVictim() != assigned;
}

Player* RaidAq40::GetSkeramHealerTank(PlayerbotAI* botAI, Player* bot)
{
    if (!PlayerbotAI::IsHeal(bot) || !bot->IsAlive())
        return nullptr;

    Group* group = bot->GetGroup();
    if (!group)
        return nullptr;

    // Same tank list the pickup assignment uses, so pairs stay aligned.
    std::vector<Player*> tanks;
    int32 myRank = -1;
    uint32 healerCount = 0;
    for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
    {
        Player* member = itr->GetSource();
        if (!member || !member->IsAlive() || !GET_PLAYERBOT_AI(member))
            continue;

        if (PlayerbotAI::IsTank(member))
            tanks.push_back(member);
        else if (PlayerbotAI::IsHeal(member))
        {
            if (member == bot)
                myRank = healerCount;

            ++healerCount;
        }
    }

    if (myRank < 0 || tanks.empty())
        return nullptr;

    return tanks[myRank % tanks.size()];
}

bool Aq40SkeramHealerFollowTrigger::IsActive()
{
    if (bot->GetMapId() != RaidAq40::MAP_TEMPLE_OF_AHNQIRAJ || !bot->IsAlive())
        return false;

    if (!IsRaidGroupInCombat(bot))
        return false;

    Creature* skeram = bot->FindNearestCreature(RaidAq40::NPC_PROPHET_SKERAM, 150.0f);
    if (!skeram || !skeram->IsInCombat())
        return false;

    Player* tank = RaidAq40::GetSkeramHealerTank(botAI, bot);
    if (!tank)
        return false;

    // Follow when far OR when platform geometry breaks line of sight.
    return bot->GetDistance(tank) > 30.0f ||
           !bot->IsWithinLOS(tank->GetPositionX(), tank->GetPositionY(), tank->GetPositionZ());
}

bool Aq40TwinsTeamSpacingTrigger::IsActive()
{
    if (bot->GetMapId() != RaidAq40::MAP_TEMPLE_OF_AHNQIRAJ || !bot->IsAlive())
        return false;

    if (!RaidAq40::IsCasterDps(bot) && !PlayerbotAI::IsHeal(bot))
        return false;

    Unit* veklor = AI_VALUE2(Unit*, "find target", "emperor vek'lor");
    Unit* veknilash = AI_VALUE2(Unit*, "find target", "emperor vek'nilash");
    if (!veklor || !veknilash || !veklor->IsInCombat())
        return false;

    // Healers split with their team: only those nearer Vek'lor's side count
    // as caster-team (the physical team keeps its own healers).
    if (PlayerbotAI::IsHeal(bot) && bot->GetDistance(veknilash) < bot->GetDistance(veklor))
        return false;

    // Only maintain spacing once the twins are actually split: while they
    // stand together (the pull, right after a swap) "35y from Vek'nilash
    // AND in cast range of Vek'lor" has no solution, and enforcing it froze
    // the whole caster team into doing nothing. Fight first, spread once
    // the tanks have made room.
    if (veklor->GetDistance(veknilash) < 55.0f)
        return false;

    return bot->GetDistance(veknilash) < RaidAq40::TWINS_TEAM_SPACING;
}

bool Aq40TwinsTankDragTrigger::IsActive()
{
    if (bot->GetMapId() != RaidAq40::MAP_TEMPLE_OF_AHNQIRAJ || !bot->IsAlive() || !PlayerbotAI::IsTank(bot))
        return false;

    Unit* veklor = AI_VALUE2(Unit*, "find target", "emperor vek'lor");
    Unit* veknilash = AI_VALUE2(Unit*, "find target", "emperor vek'nilash");
    if (!veklor || !veknilash || !veknilash->IsInCombat())
        return false;

    if (veknilash->GetVictim() != bot)
        return false;

    return veklor->GetDistance(veknilash) < RaidAq40::TWINS_SEPARATION_RANGE;
}

bool Aq40TwinsMarkTrigger::IsActive()
{
    if (bot->GetMapId() != RaidAq40::MAP_TEMPLE_OF_AHNQIRAJ || !bot->IsAlive() || !IsRaidMarkOwner(bot))
        return false;

    Group* group = bot->GetGroup();
    if (!group)
        return false;

    Unit* veklor = AI_VALUE2(Unit*, "find target", "emperor vek'lor");
    Unit* veknilash = AI_VALUE2(Unit*, "find target", "emperor vek'nilash");
    if (!veklor || !veknilash || !veknilash->IsInCombat())
        return false;

    return group->GetTargetIcon(RtiTargetValue::skullIndex) != veknilash->GetGUID() ||
           group->GetTargetIcon(RtiTargetValue::crossIndex) != veklor->GetGUID();
}

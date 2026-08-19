#include "Aq40Actions.h"

#include "Aq40Utils.h"
#include "RtiTargetValue.h"

bool Aq40ExitStomachAction::Execute(Event /*event*/)
{
    Aura* acid = bot->GetAura(RaidAq40::SPELL_DIGESTIVE_ACID);
    uint32 acidStacks = acid ? acid->GetStackAmount() : 0;

    // C'Thun only becomes vulnerable when the Flesh Tentacles in the stomach
    // die, so fight them until the acid gets dangerous.
    if (acidStacks < RaidAq40::EXIT_ACID_STACKS)
    {
        std::list<Creature*> tentacles;
        bot->GetCreatureListWithEntryInGrid(tentacles, RaidAq40::NPC_FLESH_TENTACLE, 100.0f);

        Creature* target = nullptr;
        for (Creature* tentacle : tentacles)
            if (tentacle && tentacle->IsAlive() &&
                (!target || bot->GetDistance(tentacle) < bot->GetDistance(target)))
                target = tentacle;

        if (target)
        {
            if (AI_VALUE(Unit*, "current target") != target)
                return Attack(target);

            // already fighting it - let the normal combat engine run
            return false;
        }
    }

    // Tentacles are down (or the acid is stacking up): run to the exit
    // areatrigger at the stomach edge.
    if (bot->GetDistance(RaidAq40::STOMACH_EXIT_X, RaidAq40::STOMACH_EXIT_Y, RaidAq40::STOMACH_EXIT_Z) > 4.0f)
        return MoveTo(RaidAq40::MAP_TEMPLE_OF_AHNQIRAJ,
            RaidAq40::STOMACH_EXIT_X, RaidAq40::STOMACH_EXIT_Y, RaidAq40::STOMACH_EXIT_Z);

    // Bots have no client, so walking into the areatrigger does nothing on its
    // own - fire it by hand. The core's at_cthun_stomach_exit script then
    // knocks the bot up and teleports it out over the next few seconds; hold
    // still so the "near exit trigger" check in that script stays true.
    WorldPacket packet(CMSG_AREATRIGGER);
    packet << uint32(RaidAq40::AREATRIGGER_STOMACH_EXIT);
    packet.rpos(0);
    bot->GetSession()->HandleAreaTriggerOpcode(packet);

    botAI->SetNextCheckDelay(5000);
    return true;
}

bool Aq40DodgeDarkGlareAction::Execute(Event /*event*/)
{
    Unit* eye = AI_VALUE2(Unit*, "find target", "eye of c'thun");
    if (!eye)
        return false;

    float const beamAngle = eye->GetOrientation();
    float const delta = RaidAq40::AngleDelta(eye->GetAngle(bot), beamAngle);

    // Run away from where the beam is NOW: if the sweep chases, the bot keeps
    // fleeing in the same rotational direction and outruns it (~0.23 rad/s
    // tangential at 30y vs the beam's ~0.09 rad/s); if the sweep rotates the
    // other way, the gap opens twice as fast. No direction prediction needed.
    float const away = delta >= 0.0f ? 1.0f : -1.0f;
    float const targetAngle = beamAngle + away * (std::fabs(delta) + RaidAq40::DARK_GLARE_DODGE_STEP);

    float const range = std::max(RaidAq40::DARK_GLARE_MIN_RANGE,
                                 std::min(RaidAq40::DARK_GLARE_MAX_RANGE, bot->GetDistance2d(eye)));

    float x = eye->GetPositionX() + std::cos(targetAngle) * range;
    float y = eye->GetPositionY() + std::sin(targetAngle) * range;
    float z = eye->GetPositionZ();
    bot->UpdateAllowedPositionZ(x, y, z);

    return MoveTo(RaidAq40::MAP_TEMPLE_OF_AHNQIRAJ, x, y, z, false, false, false,
                  /*exact_waypoint*/ true, MovementPriority::MOVEMENT_COMBAT, /*lessDelay*/ true);
}

bool Aq40SkeramMarkAction::Execute(Event /*event*/)
{
    Group* group = bot->GetGroup();
    if (!group)
        return false;

    // Images are TempSummons of the boss entry; the real Skeram is the only
    // non-summon. Without this, the raid splits its damage three ways.
    std::list<Creature*> skerams;
    bot->GetCreatureListWithEntryInGrid(skerams, RaidAq40::NPC_PROPHET_SKERAM, 120.0f);

    Creature* real = nullptr;
    bool imagePresent = false;
    for (Creature* skeram : skerams)
    {
        if (!skeram->IsAlive())
            continue;

        if (skeram->ToTempSummon())
            imagePresent = true;
        else
            real = skeram;
    }

    if (!real || !imagePresent)
        return false;

    if (group->GetTargetIcon(RtiTargetValue::skullIndex) == real->GetGUID())
        return false;

    group->SetTargetIcon(RtiTargetValue::skullIndex, bot->GetGUID(), real->GetGUID());
    return true;
}

bool Aq40SarturaFleeAction::Execute(Event /*event*/)
{
    Unit* danger = nullptr;
    for (auto const& guid : AI_VALUE(GuidVector, "attackers"))
    {
        Unit* unit = botAI->GetUnit(guid);
        if (!unit || !unit->IsAlive())
            continue;

        if (!unit->HasAura(RaidAq40::SPELL_SARTURA_WHIRLWIND) && !unit->HasAura(RaidAq40::SPELL_GUARD_WHIRLWIND))
            continue;

        if (bot->GetDistance(unit) > RaidAq40::WHIRLWIND_DANGER_RANGE)
            continue;

        if (!danger || bot->GetDistance(unit) < bot->GetDistance(danger))
            danger = unit;
    }

    if (!danger)
        return false;

    return MoveAway(danger, RaidAq40::WHIRLWIND_FLEE_DISTANCE);
}

bool Aq40TwinsRetargetAction::Execute(Event /*event*/)
{
    Unit* desired = RaidAq40::IsCasterDps(bot)
        ? AI_VALUE2(Unit*, "find target", "emperor vek'lor")
        : AI_VALUE2(Unit*, "find target", "emperor vek'nilash");

    if (!desired || !desired->IsAlive())
        return false;

    return Attack(desired);
}

bool Aq40TwinsTankPickupAction::Execute(Event /*event*/)
{
    Unit* veklor = AI_VALUE2(Unit*, "find target", "emperor vek'lor");
    Unit* veknilash = AI_VALUE2(Unit*, "find target", "emperor vek'nilash");

    Unit* nearest = nullptr;
    for (Unit* twin : { veklor, veknilash })
        if (twin && twin->IsAlive() && (!nearest || bot->GetDistance(twin) < bot->GetDistance(nearest)))
            nearest = twin;

    if (!nearest)
        return false;

    return Attack(nearest);
}

bool Aq40TwinsSeparateAction::Execute(Event /*event*/)
{
    Unit* veknilash = AI_VALUE2(Unit*, "find target", "emperor vek'nilash");
    if (!veknilash)
        return false;

    // Vek'lor follows his victim (teleporting past 45y), so walking away
    // from Vek'nilash drags him out of heal range.
    return MoveAway(veknilash, RaidAq40::TWINS_SEPARATION_STEP);
}

bool Aq40TwinsCasterRangeAction::Execute(Event /*event*/)
{
    Unit* veklor = AI_VALUE2(Unit*, "find target", "emperor vek'lor");
    if (!veklor)
        return false;

    return MoveAway(veklor, RaidAq40::TWINS_CASTER_FLEE_DISTANCE);
}

bool Aq40GiantClawSitAction::Execute(Event /*event*/)
{
    Creature* tentacle = bot->FindNearestCreature(RaidAq40::NPC_GIANT_CLAW_TENTACLE, 80.0f);
    if (!tentacle || !tentacle->IsAlive())
        return false;

    return MoveNear(tentacle, 2.0f, MovementPriority::MOVEMENT_COMBAT);
}

bool Aq40SkeramTankPickupAction::Execute(Event event)
{
    Unit* assigned = RaidAq40::GetSkeramPickupAssignment(botAI, bot);
    if (!assigned)
        return false;

    if (bot->GetVictim() != assigned)
        return Attack(assigned);

    std::string const tauntAction = bot->getClass() == CLASS_DRUID ? "growl" : "taunt spell";
    return botAI->DoSpecificAction(tauntAction, event, true);
}

bool Aq40SkeramHealerFollowAction::Execute(Event /*event*/)
{
    Player* tank = RaidAq40::GetSkeramHealerTank(botAI, bot);
    if (!tank)
        return false;

    return MoveNear(tank, 15.0f, MovementPriority::MOVEMENT_COMBAT);
}

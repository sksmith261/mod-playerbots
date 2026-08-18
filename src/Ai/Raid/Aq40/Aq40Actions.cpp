#include "Aq40Actions.h"

#include "Aq40Utils.h"

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

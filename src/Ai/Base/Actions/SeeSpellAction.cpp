/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "SeeSpellAction.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "Event.h"
#include "Formations.h"
#include "Group.h"
#include "Playerbots.h"
#include "RTSCValues.h"
#include "RtscAction.h"
#include "PositionValue.h"
#include "ByteBuffer.h"

std::set<uint32> const FISHING_SPELLS = {7620, 7731, 7732, 18248, 33095, 51294};

Creature* SeeSpellAction::CreateWps(Player* wpOwner, float x, float y, float z, float o, uint32 entry, Creature* /*lastWp*/,
                                    bool important)
{
    float dist = wpOwner->GetDistance(x, y, z);
    float delay = 1000.0f * dist / wpOwner->GetSpeed(MOVE_RUN) + sPlayerbotAIConfig.reactDelay;

    if (!important)
        delay *= 0.25;

    Creature* wpCreature = wpOwner->SummonCreature(entry, x, y, z - 1, o, TEMPSUMMON_TIMED_DESPAWN, delay);
    if (!important)
        wpCreature->SetObjectScale(0.2f);

    return wpCreature;
}

bool SeeSpellAction::Execute(Event event)
{
    // RTSC packet data
    WorldPacket p(event.getPacket());
    uint8 castCount;
    uint32 spellId;
    uint8 castFlags;

    // check RTSC header size = castCount (uint8) + spellId (uint32) + castFlags (uint8)
    uint32 const rtscHeaderSize = sizeof(uint8) + sizeof(uint32) + sizeof(uint8);
    if (p.size() < rtscHeaderSize)
    {
        LOG_WARN("playerbots", "SeeSpellAction: Corrupt RTSC packet size={}, expected>={}", p.size(), rtscHeaderSize);
        return false;
    }

    Player* master = botAI->GetMaster();
    if (!master)
        return false;

    // read RTSC packet data
    p.rpos(0); // set read position to start
    p >> castCount >> spellId >> castFlags;

    // if (!botAI->HasStrategy("RTSC", botAI->GetState()))
    //     return false;

    if (FISHING_SPELLS.find(spellId) != FISHING_SPELLS.end())
    {
        if (AI_VALUE(bool, "can fish") && sPlayerbotAIConfig.enableFishingWithMaster)
        {
            botAI->ChangeStrategy("+master fishing", BOT_STATE_NON_COMBAT);
            return true;
        }
        return false;
    }

    if (spellId != RTSC_MOVE_SPELL)
        return false;

    // should not throw exception,just defensive measure to prevent any crashes when core function breaks.
    SpellCastTargets targets;
    try
    {
        targets.Read(p, master);
        if (!targets.GetDst())
        {
            // do not dereference a null destination; ignore malformed RTSC packets instead of crashing
            LOG_WARN("playerbots", "SeeSpellAction: (malformed) RTSC payload does not contain full targets data");
            return false;
        }
    }
    catch (ByteBufferException const&)
    {
        // ignore malformed RTSC packets instead of crashing
        LOG_WARN("playerbots", "SeeSpellAction: Failed deserialization (malformed) RTSC payload");
        return false;
    }

    WorldPosition spellPosition(master->GetMapId(), targets.GetDst()->_position);
    SET_AI_VALUE(WorldPosition, "see spell location", spellPosition);

    bool selected = AI_VALUE(bool, "RTSC selected");
    bool inRange = spellPosition.distance(bot) <= 10;
    std::string const nextAction = AI_VALUE(std::string, "RTSC next spell action");

    if (nextAction.empty())
    {
        if (!inRange && selected)
            master->SendPlaySpellVisual(bot->GetGUID(), 6372);
        else if (inRange && !selected)
            master->SendPlaySpellVisual(bot->GetGUID(), 5036);

        SET_AI_VALUE(bool, "RTSC selected", inRange);

        if (selected)
            return MoveToSpell(spellPosition);

        return inRange;
    }
    else if (nextAction == "move")
    {
        return MoveToSpell(spellPosition);
    }
    else if (nextAction.find("save ") != std::string::npos)
    {
        std::string locationName;
        if (nextAction.find("save selected ") != std::string::npos)
        {
            if (!selected)
                return false;

            locationName = nextAction.substr(14);
        }
        else
            locationName = nextAction.substr(5);

        SetFormationOffset(spellPosition);

        SET_AI_VALUE2(WorldPosition, "RTSC saved location", locationName, spellPosition);

        Creature* wpCreature =
            bot->SummonCreature(15631, spellPosition.GetPositionX(), spellPosition.GetPositionY(), spellPosition.GetPositionZ(),
                                spellPosition.GetOrientation(), TEMPSUMMON_TIMED_DESPAWN, 2000.0f);
        wpCreature->SetObjectScale(0.5f);
        RESET_AI_VALUE(std::string, "RTSC next spell action");

        return true;
    }
    else if (nextAction.rfind("spread", 0) == 0 || nextAction == "stack" || nextAction == "goto")
    {
        // Deliberately no value reset: the armed set must stay stable while
        // bots drain the click packet on different ticks, and it lets the
        // master re-click to re-form the ring. "follow"/"stay"/"rtsc reset"
        // disarm it.
        return MoveToClickFormation(spellPosition, nextAction);
    }

    return false;
}

bool SeeSpellAction::MoveToClickFormation(WorldPosition& center, std::string const& armed)
{
    std::string const token = armed.substr(0, armed.find(' '));
    float const gap = token == "spread" && armed.size() > 7 ? atof(armed.substr(7).c_str()) : 0.0f;

    Player* master = botAI->GetMaster();
    Group* group = bot->GetGroup();
    if (!master || !group)
        return false;

    // Self-qualification: same rules every bot applies to every member below,
    // so slot assignments never collide.
    if (bot->GetMapId() != center.GetMapId() || !bot->IsAlive() || bot->GetVehicle() || bot->IsInFlight())
        return false;

    // Roster: group-iteration order is the same shared list for every bot, and
    // the armed value persists across the click, so each bot independently
    // computes an identical assignment (same idea as MountDrakeAction).
    uint32 n = 0;
    int32 myRank = -1;
    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (!member || !member->IsInWorld())
            continue;

        PlayerbotAI* memberAI = GET_PLAYERBOT_AI(member);
        if (!memberAI || memberAI->GetMaster() != master)
            continue;

        if (member->GetMapId() != center.GetMapId())
            continue;

        if (!member->IsAlive() || member->GetVehicle() || member->IsInFlight())
            continue;

        std::string const memberArmed =
            memberAI->GetAiObjectContext()->GetValue<std::string>("RTSC next spell action")->Get();
        if (memberArmed.substr(0, memberArmed.find(' ')) != token)
            continue;

        if (member == bot)
            myRank = n;

        ++n;
    }

    if (myRank < 0 || !n)
        return false;

    float radius;
    if (token == "spread")
        radius = n == 1 ? std::max(2.0f, gap) : std::max({2.0f, gap / 2.0f, n * gap / (2.0f * static_cast<float>(M_PI))});
    else if (token == "stack")
        radius = n == 1 ? 0.0f : 1.5f;
    else  // goto
        radius = n == 1 ? 0.0f : 3.0f;

    // Full 360-degree ring, anchored on the master->click direction so slots
    // are stable between re-clicks from the same spot.
    float const base = atan2(center.GetPositionY() - master->GetPositionY(), center.GetPositionX() - master->GetPositionX());
    float const angle = base + 2.0f * static_cast<float>(M_PI) * myRank / n;

    float x = center.GetPositionX() + cos(angle) * radius;
    float y = center.GetPositionY() + sin(angle) * radius;
    float z = center.GetPositionZ();

    Map* map = bot->GetMap();
    if (!map->CheckCollisionAndGetValidCoords(bot, center.GetPositionX(), center.GetPositionY(), center.GetPositionZ(), x, y, z))
    {
        x = center.GetPositionX() + cos(angle) * radius;
        y = center.GetPositionY() + sin(angle) * radius;
        z = center.GetPositionZ();
        bot->UpdateAllowedPositionZ(x, y, z);
    }

    // Hold the slot: stay anchor at the destination, stay strategy on. +stay
    // removes follow automatically (exclusive movement strategy group).
    // Mirrors StayChatShortcutAction minus botAI->Reset(), which would clear
    // values mid-click.
    PositionMap& posMap = AI_VALUE(PositionMap&, "position");
    PositionInfo pos = posMap["stay"];
    pos.Set(x, y, z, center.GetMapId());
    posMap["stay"] = pos;
    pos = posMap["return"];
    pos.Set(x, y, z, center.GetMapId());
    posMap["return"] = pos;
    botAI->ChangeStrategy("+stay,-passive,-move from group", BOT_STATE_NON_COMBAT);
    botAI->ChangeStrategy("+stay,-follow,-passive,-move from group", BOT_STATE_COMBAT);

    // One marker pin at the center, not one per bot.
    if (!myRank)
    {
        if (Creature* wpCreature = bot->SummonCreature(15631, center.GetPositionX(), center.GetPositionY(), center.GetPositionZ(), 0.0f,
                                                       TEMPSUMMON_TIMED_DESPAWN, 2000.0f))
            wpCreature->SetObjectScale(0.5f);
    }

    bool const moved = MoveTo(center.GetMapId(), x, y, z, false, false, false, /*exact_waypoint*/ true,
                              MovementPriority::MOVEMENT_COMBAT);
    if (!moved)
        botAI->TellError("I cannot reach that spot");

    return moved;
}

bool SeeSpellAction::SelectSpell(WorldPosition& spellPosition)
{
    Player* master = botAI->GetMaster();
    if (spellPosition.distance(bot) <= 5 || AI_VALUE(bool, "RTSC selected"))
    {
        SET_AI_VALUE(bool, "RTSC selected", true);
        master->SendPlaySpellVisual(bot->GetGUID(), 5036);
    }

    return true;
}

bool SeeSpellAction::MoveToSpell(WorldPosition& spellPosition, bool inFormation)
{
    if (inFormation)
        SetFormationOffset(spellPosition);

    if (botAI->HasStrategy("stay", botAI->GetState()))
    {
        PositionMap& posMap = AI_VALUE(PositionMap&, "position");
        PositionInfo stayPosition = posMap["stay"];

        stayPosition.Set(spellPosition.GetPositionX(), spellPosition.GetPositionY(), spellPosition.GetPositionZ(), spellPosition.GetMapId());
        posMap["stay"] = stayPosition;
    }

    if (bot->IsWithinLOS(spellPosition.GetPositionX(), spellPosition.GetPositionY(), spellPosition.GetPositionZ()))
        return MoveNear(spellPosition.GetMapId(), spellPosition.GetPositionX(), spellPosition.GetPositionY(), spellPosition.GetPositionZ(), 0);

    return MoveTo(spellPosition.GetMapId(), spellPosition.GetPositionX(), spellPosition.GetPositionY(), spellPosition.GetPositionZ(), false,
                  false);
}

void SeeSpellAction::SetFormationOffset(WorldPosition& spellPosition)
{
    Player* master = botAI->GetMaster();

    Formation* formation = AI_VALUE(Formation*, "formation");

    WorldLocation formationLocation = formation->GetLocation();

    if (formationLocation.GetPositionX() != 0 || formationLocation.GetPositionY() != 0)
    {
        spellPosition -= WorldPosition(master);
        spellPosition += formationLocation;
    }
}

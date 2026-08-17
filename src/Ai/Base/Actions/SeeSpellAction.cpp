/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "SeeSpellAction.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
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
    else if (nextAction == "path")
    {
        // Recording mode is multi-click by design: every click appends a
        // waypoint and the command stays armed until "path go"/"path clear".
        std::string path = AI_VALUE(std::string, "click path");
        if (path.rfind("rec", 0) != 0)
            path = "rec";

        char wpBuf[96];
        snprintf(wpBuf, sizeof(wpBuf), ";%.2f,%.2f,%.2f", spellPosition.GetPositionX(),
                 spellPosition.GetPositionY(), spellPosition.GetPositionZ());
        SET_AI_VALUE(std::string, "click path", path + wpBuf);

        // One marker per click, not one per bot: only the first armed group
        // bot (in shared iteration order) pins it.
        bool firstArmed = true;
        if (Group* group = bot->GetGroup())
        {
            for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
            {
                Player* member = ref->GetSource();
                if (member == bot)
                    break;  // reached myself with no earlier armed bot found

                if (!member)
                    continue;

                PlayerbotAI* memberAI = GET_PLAYERBOT_AI(member);
                if (memberAI && memberAI->GetMaster() == master &&
                    memberAI->GetAiObjectContext()->GetValue<std::string>("RTSC next spell action")->Get() == "path")
                {
                    firstArmed = false;
                    break;
                }
            }
        }

        if (firstArmed)
        {
            if (Creature* wpCreature = bot->SummonCreature(15631, spellPosition.GetPositionX(), spellPosition.GetPositionY(),
                                                           spellPosition.GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN, 5000.0f))
                wpCreature->SetObjectScale(0.5f);
        }

        return true;
    }
    else if (IsClickCommand(nextAction))
    {
        // One-shot per arm: a click marks the command spent (tagged with the
        // click position) instead of resetting it, so bots that drain this
        // click's packet on later ticks still count this bot in their slot
        // math, while any future click ignores it. Re-forming on a new spot
        // means issuing the command again — otherwise a raid armed by an
        // earlier unscoped command would move again on every later click
        // (e.g. one meant only for "@group2 goto").
        if (nextAction.find('|') != std::string::npos)
            return false;  // spent on an earlier click

        return MoveToClickFormation(spellPosition, nextAction);
    }

    return false;
}

std::string SeeSpellAction::ClickToken(std::string const& armed)
{
    return armed.substr(0, armed.find_first_of(" |"));
}

bool SeeSpellAction::IsClickCommand(std::string const& armed)
{
    std::string const token = ClickToken(armed);
    return token == "spread" || token == "stack" || token == "goto" || token == "line" || token == "sweep";
}

bool SeeSpellAction::MoveToClickFormation(WorldPosition& center, std::string const& armed)
{
    std::string const token = ClickToken(armed);
    float gap = 3.0f;
    size_t const space = armed.find(' ');
    if (space != std::string::npos)
        gap = atof(armed.substr(space + 1).c_str());

    // Identifies this click for spent-marking; every bot formats the same
    // packet coordinates identically, so the key is consistent bot-to-bot.
    char clickKeyBuf[64];
    snprintf(clickKeyBuf, sizeof(clickKeyBuf), "%.1f,%.1f", center.GetPositionX(), center.GetPositionY());
    std::string const clickKey(clickKeyBuf);

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
        if (ClickToken(memberArmed) != token)
            continue;

        // Members spent on THIS click still count (they acted on it a tick
        // or two ago); members spent on a different click are older waves.
        size_t const memberPipe = memberArmed.find('|');
        if (memberPipe != std::string::npos && memberArmed.substr(memberPipe + 1) != clickKey)
            continue;

        if (member == bot)
            myRank = n;

        ++n;
    }

    if (myRank < 0 || !n)
        return false;

    // Mark spent before moving: from here on this bot has acted on this click.
    SET_AI_VALUE(std::string, "RTSC next spell action", armed + "|" + clickKey);

    float radius;
    if (token == "spread")
        radius = n == 1 ? std::max(2.0f, gap) : std::max({2.0f, gap / 2.0f, n * gap / (2.0f * static_cast<float>(M_PI))});
    else if (token == "stack")
        radius = n == 1 ? 0.0f : 1.5f;
    else  // goto / sweep / line (line uses gap directly, not radius)
        radius = n == 1 ? 0.0f : 3.0f;

    // Anchored on the master->click direction so slots are stable between
    // clicks from the same spot.
    float const base = atan2(center.GetPositionY() - master->GetPositionY(), center.GetPositionX() - master->GetPositionX());

    float x, y;
    float z = center.GetPositionZ();
    if (token == "line")
    {
        // A line through the click, perpendicular to the approach direction,
        // centered on the click with gap yards between adjacent slots.
        float const perp = base + static_cast<float>(M_PI) / 2.0f;
        float const offset = gap * (myRank - (n - 1) / 2.0f);
        x = center.GetPositionX() + cos(perp) * offset;
        y = center.GetPositionY() + sin(perp) * offset;
    }
    else
    {
        // Full 360-degree ring.
        float const angle = base + 2.0f * static_cast<float>(M_PI) * myRank / n;
        x = center.GetPositionX() + cos(angle) * radius;
        y = center.GetPositionY() + sin(angle) * radius;
    }

    float const desiredX = x;
    float const desiredY = y;

    Map* map = bot->GetMap();
    if (!map->CheckCollisionAndGetValidCoords(bot, center.GetPositionX(), center.GetPositionY(), center.GetPositionZ(), x, y, z))
    {
        x = desiredX;
        y = desiredY;
        z = center.GetPositionZ();
        bot->UpdateAllowedPositionZ(x, y, z);
    }

    PositionMap& posMap = AI_VALUE(PositionMap&, "position");
    if (token == "sweep")
    {
        // Guard the slot instead of staying on it: the guard strategy holds
        // the area but lets the bot chase and engage freely, and grind makes
        // it proactively clear hostiles around the point.
        PositionInfo pos = posMap["guard"];
        pos.Set(x, y, z, center.GetMapId());
        posMap["guard"] = pos;
        botAI->ChangeStrategy("+guard,+grind,-passive,-move from group", BOT_STATE_NON_COMBAT);
        botAI->ChangeStrategy("+guard,-follow,-passive,-move from group", BOT_STATE_COMBAT);
    }
    else
    {
        // Hold the slot: stay anchor at the destination, stay strategy on.
        // +stay removes follow automatically (exclusive movement strategy
        // group). Mirrors StayChatShortcutAction minus botAI->Reset(), which
        // would clear values mid-click.
        PositionInfo pos = posMap["stay"];
        pos.Set(x, y, z, center.GetMapId());
        posMap["stay"] = pos;
        pos = posMap["return"];
        pos.Set(x, y, z, center.GetMapId());
        posMap["return"] = pos;
        botAI->ChangeStrategy("+stay,-passive,-move from group", BOT_STATE_NON_COMBAT);
        botAI->ChangeStrategy("+stay,-follow,-passive,-move from group", BOT_STATE_COMBAT);
    }

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

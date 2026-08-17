/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "ClickCommandActions.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <vector>

#include "Event.h"
#include "Playerbots.h"
#include "PositionValue.h"
#include "RtscAction.h"

void ClickFormationCommandAction::EnsureMasterHasClickSpell()
{
    Player* master = botAI->GetMaster();
    if (!master || master->HasSpell(RTSC_MOVE_SPELL))
        return;

    master->learnSpell(RTSC_MOVE_SPELL, false);
    botAI->TellMasterNoFacing("RTS control enabled.");
    botAI->TellMasterNoFacing("Aedm (Awesome energetic do move) spell trained.");
}

bool ClickFormationCommandAction::Execute(Event event)
{
    Player* master = botAI->GetMaster();
    if (!master)
        return false;

    std::string const token = getName();  // "spread" | "line" | "stack" | "goto" | "sweep"
    std::string armed = token;

    if (token == "spread" || token == "line")
    {
        float gap = 3.0f;
        std::string const param = event.getParam();
        if (!param.empty() && sscanf(param.c_str(), "%f", &gap) != 1)
        {
            botAI->TellMasterNoFacing("Usage: " + token + " <yards between bots> (e.g. " + token + " 5), then click the ground.");
            return false;
        }

        float const clamped = std::clamp(gap, 0.5f, 50.0f);
        std::ostringstream out;
        if (clamped != gap)
        {
            out << "Spacing clamped to " << clamped << " yards.";
            botAI->TellMasterNoFacing(out.str());
            out.str("");
        }

        out << token << " ";
        out.precision(2);
        out << std::fixed << clamped;
        armed = out.str();
    }

    EnsureMasterHasClickSpell();

    SET_AI_VALUE(std::string, "RTSC next spell action", armed);
    botAI->TellMasterNoFacing("Click a spot on the ground with the Aedm spell.");

    return true;
}

bool PathCommandAction::Execute(Event event)
{
    Player* master = botAI->GetMaster();
    if (!master)
        return false;

    std::string const param = event.getParam();

    if (param == "clear")
    {
        RESET_AI_VALUE(std::string, "click path");
        if (AI_VALUE(std::string, "RTSC next spell action") == "path")
            RESET_AI_VALUE(std::string, "RTSC next spell action");

        botAI->TellMasterNoFacing("Path cleared.");
        return true;
    }

    if (param == "go")
    {
        std::string const path = AI_VALUE(std::string, "click path");
        if (path.rfind("rec", 0) != 0 || path.find(';') == std::string::npos)
        {
            botAI->TellMasterNoFacing("No waypoints recorded - say 'path' and click spots first.");
            return false;
        }

        // "rec;w1;w2" -> "go,0;w1;w2"
        SET_AI_VALUE(std::string, "click path", "go,0" + path.substr(3));
        if (AI_VALUE(std::string, "RTSC next spell action") == "path")
            RESET_AI_VALUE(std::string, "RTSC next spell action");

        // Nothing else may own movement while the path trigger drives it.
        botAI->ChangeStrategy("-follow,-stay,-passive,-move from group", BOT_STATE_NON_COMBAT);
        botAI->TellMasterNoFacing("Walking the path.");
        return true;
    }

    if (!param.empty())
    {
        botAI->TellMasterNoFacing("Usage: path (record clicks) | path go | path clear");
        return false;
    }

    // Start or continue recording. Re-issuing "path" keeps the waypoints
    // collected so far, so one addon button can be pressed per waypoint.
    if (AI_VALUE(std::string, "click path").rfind("rec", 0) != 0)
        SET_AI_VALUE(std::string, "click path", "rec");

    EnsureMasterHasClickSpell();

    SET_AI_VALUE(std::string, "RTSC next spell action", "path");
    botAI->TellMasterNoFacing("Path recording: click waypoints with Aedm, then say 'path go'.");

    return true;
}

bool ClickPathNextTrigger::IsActive()
{
    return AI_VALUE(std::string, "click path").rfind("go,", 0) == 0;
}

bool ClickPathNextAction::Execute(Event /*event*/)
{
    std::string const path = AI_VALUE(std::string, "click path");
    if (path.rfind("go,", 0) != 0)
        return false;

    size_t const firstSemi = path.find(';');
    if (firstSemi == std::string::npos)
    {
        RESET_AI_VALUE(std::string, "click path");
        return false;
    }

    uint32 idx = atoi(path.substr(3, firstSemi - 3).c_str());

    std::vector<std::string> waypoints;
    std::istringstream stream(path.substr(firstSemi + 1));
    std::string wp;
    while (std::getline(stream, wp, ';'))
        if (!wp.empty())
            waypoints.push_back(wp);

    auto finishPath = [&]() -> bool
    {
        RESET_AI_VALUE(std::string, "click path");

        PositionMap& posMap = AI_VALUE(PositionMap&, "position");
        PositionInfo pos = posMap["stay"];
        pos.Set(bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ(), bot->GetMapId());
        posMap["stay"] = pos;
        pos = posMap["return"];
        pos.Set(bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ(), bot->GetMapId());
        posMap["return"] = pos;
        botAI->ChangeStrategy("+stay,-passive,-move from group", BOT_STATE_NON_COMBAT);
        botAI->ChangeStrategy("+stay,-follow,-passive,-move from group", BOT_STATE_COMBAT);

        botAI->TellMaster("Path complete.");
        return true;
    };

    if (idx >= waypoints.size())
        return finishPath();

    float x = 0.0f, y = 0.0f, z = 0.0f;
    if (sscanf(waypoints[idx].c_str(), "%f,%f,%f", &x, &y, &z) != 3)
        return finishPath();

    if (bot->GetDistance(x, y, z) < 4.0f)
    {
        ++idx;
        if (idx >= waypoints.size())
            return finishPath();

        SET_AI_VALUE(std::string, "click path",
                     "go," + std::to_string(idx) + path.substr(firstSemi));
        if (sscanf(waypoints[idx].c_str(), "%f,%f,%f", &x, &y, &z) != 3)
            return finishPath();
    }

    // Small ring offset so bots don't fight for the same square yard; normal
    // priority so combat movement wins and the walk resumes afterwards.
    return MoveNear(bot->GetMapId(), x, y, z, 2.0f, MovementPriority::MOVEMENT_NORMAL);
}

/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "ClickCommandActions.h"

#include <algorithm>
#include <cstdio>
#include <sstream>

#include "Event.h"
#include "Playerbots.h"
#include "RtscAction.h"

bool ClickFormationCommandAction::Execute(Event event)
{
    Player* master = botAI->GetMaster();
    if (!master)
        return false;

    std::string const token = getName();  // "spread" | "stack" | "goto"
    std::string armed = token;

    if (token == "spread")
    {
        float gap = 3.0f;
        std::string const param = event.getParam();
        if (!param.empty() && sscanf(param.c_str(), "%f", &gap) != 1)
        {
            botAI->TellMasterNoFacing("Usage: spread <yards between bots> (e.g. spread 5), then click the ground.");
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

        out << "spread ";
        out.precision(2);
        out << std::fixed << clamped;
        armed = out.str();
    }

    if (!master->HasSpell(RTSC_MOVE_SPELL))
    {
        master->learnSpell(RTSC_MOVE_SPELL, false);
        botAI->TellMasterNoFacing("RTS control enabled.");
        botAI->TellMasterNoFacing("Aedm (Awesome energetic do move) spell trained.");
    }

    SET_AI_VALUE(std::string, "RTSC next spell action", armed);
    botAI->TellMasterNoFacing("Click a spot on the ground with the Aedm spell.");

    return true;
}

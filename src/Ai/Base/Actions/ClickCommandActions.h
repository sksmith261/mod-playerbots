/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_CLICKCOMMANDACTIONS_H
#define PLAYERBOTS_CLICKCOMMANDACTIONS_H

#include "Action.h"

class PlayerbotAI;

// Arms the RTSC ground-click pipeline with a formation command. The command
// name doubles as the armed token stored in "RTSC next spell action"; the
// click itself is consumed by SeeSpellAction::MoveToClickFormation.
//   spread <x>  - circle around the clicked point, x yards between bots
//   stack       - tight ring on the clicked point
//   goto        - loose ring on the clicked point (compose with @group2 etc.)
class ClickFormationCommandAction : public Action
{
public:
    ClickFormationCommandAction(PlayerbotAI* botAI, std::string const name) : Action(botAI, name) {}

    bool Execute(Event event) override;
};

class SpreadCommandAction : public ClickFormationCommandAction
{
public:
    SpreadCommandAction(PlayerbotAI* botAI) : ClickFormationCommandAction(botAI, "spread") {}
};

class StackCommandAction : public ClickFormationCommandAction
{
public:
    StackCommandAction(PlayerbotAI* botAI) : ClickFormationCommandAction(botAI, "stack") {}
};

class GotoCommandAction : public ClickFormationCommandAction
{
public:
    GotoCommandAction(PlayerbotAI* botAI) : ClickFormationCommandAction(botAI, "goto") {}
};

#endif

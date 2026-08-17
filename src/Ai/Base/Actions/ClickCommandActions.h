/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_CLICKCOMMANDACTIONS_H
#define PLAYERBOTS_CLICKCOMMANDACTIONS_H

#include "MovementActions.h"
#include "Trigger.h"

class PlayerbotAI;

// Arms the RTSC ground-click pipeline with a formation command. The command
// name doubles as the armed token stored in "RTSC next spell action"; the
// click itself is consumed by SeeSpellAction::MoveToClickFormation.
//   spread <x>  - circle around the clicked point, x yards between bots
//   line <x>    - line through the clicked point, x yards between bots
//   stack       - tight ring on the clicked point
//   goto        - loose ring on the clicked point (compose with @group2 etc.)
//   sweep       - like goto, but bots guard the point and engage freely
class ClickFormationCommandAction : public Action
{
public:
    ClickFormationCommandAction(PlayerbotAI* botAI, std::string const name) : Action(botAI, name) {}

    bool Execute(Event event) override;

protected:
    void EnsureMasterHasClickSpell();
};

class SpreadCommandAction : public ClickFormationCommandAction
{
public:
    SpreadCommandAction(PlayerbotAI* botAI) : ClickFormationCommandAction(botAI, "spread") {}
};

class LineCommandAction : public ClickFormationCommandAction
{
public:
    LineCommandAction(PlayerbotAI* botAI) : ClickFormationCommandAction(botAI, "line") {}
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

class SweepCommandAction : public ClickFormationCommandAction
{
public:
    SweepCommandAction(PlayerbotAI* botAI) : ClickFormationCommandAction(botAI, "sweep") {}
};

// "path"       - start/continue recording: each Aedm click adds a waypoint
// "path go"    - walk the recorded waypoints in order, hold at the last one
// "path clear" - forget the recorded path / stop walking
class PathCommandAction : public ClickFormationCommandAction
{
public:
    PathCommandAction(PlayerbotAI* botAI) : ClickFormationCommandAction(botAI, "path") {}

    bool Execute(Event event) override;
};

// Fires while a "click path" value is in the go state; advances the bot
// along its recorded waypoints.
class ClickPathNextTrigger : public Trigger
{
public:
    ClickPathNextTrigger(PlayerbotAI* botAI) : Trigger(botAI, "click path next", 1) {}

    bool IsActive() override;
};

class ClickPathNextAction : public MovementAction
{
public:
    ClickPathNextAction(PlayerbotAI* botAI) : MovementAction(botAI, "click path next") {}

    bool Execute(Event event) override;
};

#endif

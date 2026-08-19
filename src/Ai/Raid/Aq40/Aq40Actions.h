#ifndef PLAYERBOTS_AQ40ACTIONS_H
#define PLAYERBOTS_AQ40ACTIONS_H

#include "AttackAction.h"
#include "MovementActions.h"
#include "PlayerbotAI.h"
#include "Playerbots.h"

class Aq40ExitStomachAction : public AttackAction
{
public:
    Aq40ExitStomachAction(PlayerbotAI* botAI, std::string const name = "aq40 exit stomach") : AttackAction(botAI, name) {}
    bool Execute(Event event) override;
};

// Run tangentially around the Eye of C'Thun, away from the Dark Glare beam,
// keeping the current engagement distance.
class Aq40DodgeDarkGlareAction : public MovementAction
{
public:
    Aq40DodgeDarkGlareAction(PlayerbotAI* botAI, std::string const name = "aq40 dodge dark glare")
        : MovementAction(botAI, name) {}
    bool Execute(Event event) override;
};

// Skull the real Skeram (the only non-TempSummon of the entry).
class Aq40SkeramMarkAction : public Action
{
public:
    Aq40SkeramMarkAction(PlayerbotAI* botAI, std::string const name = "aq40 skeram mark")
        : Action(botAI, name) {}
    bool Execute(Event event) override;
};

// Run out of whirlwind range.
class Aq40SarturaFleeAction : public MovementAction
{
public:
    Aq40SarturaFleeAction(PlayerbotAI* botAI, std::string const name = "aq40 sartura flee")
        : MovementAction(botAI, name) {}
    bool Execute(Event event) override;
};

// Tank grabs the nearest twin (post-teleport pickup).
class Aq40TwinsTankPickupAction : public AttackAction
{
public:
    Aq40TwinsTankPickupAction(PlayerbotAI* botAI, std::string const name = "aq40 twins tank pickup")
        : AttackAction(botAI, name) {}
    bool Execute(Event event) override;
};

// Drag Vek'lor away from his brother (he follows his victim within 45y).
class Aq40TwinsSeparateAction : public MovementAction
{
public:
    Aq40TwinsSeparateAction(PlayerbotAI* botAI, std::string const name = "aq40 twins separate")
        : MovementAction(botAI, name) {}
    bool Execute(Event event) override;
};

// Step out of Vek'lor's Arcane Burst range.
class Aq40TwinsCasterRangeAction : public MovementAction
{
public:
    Aq40TwinsCasterRangeAction(PlayerbotAI* botAI, std::string const name = "aq40 twins caster range")
        : MovementAction(botAI, name) {}
    bool Execute(Event event) override;
};

// Switch a DPS bot onto the twin its damage type can hurt.
class Aq40TwinsRetargetAction : public AttackAction
{
public:
    Aq40TwinsRetargetAction(PlayerbotAI* botAI, std::string const name = "aq40 twins retarget")
        : AttackAction(botAI, name) {}
    bool Execute(Event event) override;
};

// Stand on the Giant Claw Tentacle so it cannot full-heal and resubmerge.
class Aq40GiantClawSitAction : public MovementAction
{
public:
    Aq40GiantClawSitAction(PlayerbotAI* botAI, std::string const name = "aq40 giant claw sitter")
        : MovementAction(botAI, name) {}
    bool Execute(Event event) override;
};


// Grab this tank's assigned untanked Skeram (see the paired trigger).
class Aq40SkeramTankPickupAction : public AttackAction
{
public:
    Aq40SkeramTankPickupAction(PlayerbotAI* botAI, std::string const name = "aq40 skeram tank pickup")
        : AttackAction(botAI, name) {}
    bool Execute(Event event) override;
};

#endif

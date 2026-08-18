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

// Switch a DPS bot onto the twin its damage type can hurt.
class Aq40TwinsRetargetAction : public AttackAction
{
public:
    Aq40TwinsRetargetAction(PlayerbotAI* botAI, std::string const name = "aq40 twins retarget")
        : AttackAction(botAI, name) {}
    bool Execute(Event event) override;
};
#endif

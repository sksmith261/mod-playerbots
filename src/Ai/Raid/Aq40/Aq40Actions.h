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
#endif

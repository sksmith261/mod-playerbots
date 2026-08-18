#ifndef PLAYERBOTS_AQ40ACTIONS_H
#define PLAYERBOTS_AQ40ACTIONS_H

#include "AttackAction.h"
#include "PlayerbotAI.h"
#include "Playerbots.h"

class Aq40ExitStomachAction : public AttackAction
{
public:
    Aq40ExitStomachAction(PlayerbotAI* botAI, std::string const name = "aq40 exit stomach") : AttackAction(botAI, name) {}
    bool Execute(Event event) override;
};
#endif

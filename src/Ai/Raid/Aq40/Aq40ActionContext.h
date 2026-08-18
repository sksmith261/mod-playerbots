#ifndef PLAYERBOTS_AQ40ACTIONCONTEXT_H
#define PLAYERBOTS_AQ40ACTIONCONTEXT_H

#include "Action.h"
#include "NamedObjectContext.h"
#include "Aq40Actions.h"

class RaidAq40ActionContext : public NamedObjectContext<Action>
{
public:
    RaidAq40ActionContext()
    {
        creators["aq40 exit stomach"] = &RaidAq40ActionContext::exit_stomach;
        creators["aq40 dodge dark glare"] = &RaidAq40ActionContext::dodge_dark_glare;
    }

private:
    static Action* exit_stomach(PlayerbotAI* ai) { return new Aq40ExitStomachAction(ai); }
    static Action* dodge_dark_glare(PlayerbotAI* ai) { return new Aq40DodgeDarkGlareAction(ai); }
};

#endif

#ifndef PLAYERBOTS_AQ40TRIGGERCONTEXT_H
#define PLAYERBOTS_AQ40TRIGGERCONTEXT_H

#include "NamedObjectContext.h"
#include "Aq40Triggers.h"

class RaidAq40TriggerContext : public NamedObjectContext<Trigger>
{
public:
    RaidAq40TriggerContext()
    {
        creators["aq40 in stomach"] = &RaidAq40TriggerContext::in_stomach;
    }

private:
    static Trigger* in_stomach(PlayerbotAI* ai) { return new Aq40InStomachTrigger(ai); }
};

#endif

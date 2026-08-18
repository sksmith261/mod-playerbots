#ifndef PLAYERBOTS_AQ40TRIGGERS_H
#define PLAYERBOTS_AQ40TRIGGERS_H

#include "PlayerbotAI.h"
#include "Playerbots.h"
#include "Trigger.h"

class Aq40InStomachTrigger : public Trigger
{
public:
    Aq40InStomachTrigger(PlayerbotAI* botAI) : Trigger(botAI, "aq40 in stomach") {}
    bool IsActive() override;
};
#endif

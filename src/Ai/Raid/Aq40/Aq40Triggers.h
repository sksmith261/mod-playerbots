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

// The Eye of C'Thun is sweeping Dark Glare and the beam is closing on this
// bot's angular position.
class Aq40DarkGlareTrigger : public Trigger
{
public:
    Aq40DarkGlareTrigger(PlayerbotAI* botAI) : Trigger(botAI, "aq40 dark glare") {}
    bool IsActive() override;
};
#endif

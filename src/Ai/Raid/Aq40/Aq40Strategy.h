#ifndef PLAYERBOTS_AQ40STRATEGY_H
#define PLAYERBOTS_AQ40STRATEGY_H

#include "Strategy.h"

class RaidAq40Strategy : public Strategy
{
public:
    RaidAq40Strategy(PlayerbotAI* ai) : Strategy(ai) {}
    virtual std::string const getName() override { return "aq40"; }
    virtual void InitTriggers(std::vector<TriggerNode*>& triggers) override;
};

#endif

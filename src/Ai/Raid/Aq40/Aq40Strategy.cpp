#include "Aq40Strategy.h"

#include "Strategy.h"

void RaidAq40Strategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    triggers.push_back(
        new TriggerNode("aq40 in stomach",
            { NextAction("aq40 exit stomach", ACTION_RAID) }));
}

#include "Aq40Strategy.h"

#include "Strategy.h"

void RaidAq40Strategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    triggers.push_back(
        new TriggerNode("aq40 in stomach",
            { NextAction("aq40 exit stomach", ACTION_RAID) }));

    triggers.push_back(
        new TriggerNode("aq40 dark glare",
            { NextAction("aq40 dodge dark glare", ACTION_RAID) }));

    triggers.push_back(
        new TriggerNode("aq40 skeram mark", { NextAction("aq40 skeram mark", ACTION_RAID) }));
    triggers.push_back(
        new TriggerNode("aq40 sartura whirlwind", { NextAction("aq40 sartura flee", ACTION_RAID) }));
    triggers.push_back(
        new TriggerNode("aq40 sartura mark", { NextAction("aq40 sartura mark", ACTION_RAID) }));
    triggers.push_back(
        new TriggerNode("aq40 bug trio", { NextAction("aq40 bug trio mark", ACTION_RAID) }));
    triggers.push_back(
        new TriggerNode("aq40 fankriss worms", { NextAction("aq40 fankriss worm mark", ACTION_RAID) }));
    triggers.push_back(
        new TriggerNode("aq40 huhuran frenzy", { NextAction("tranquilizing shot", ACTION_RAID) }));
    triggers.push_back(
        new TriggerNode("aq40 twins wrong target", { NextAction("aq40 twins retarget", ACTION_RAID) }));
}

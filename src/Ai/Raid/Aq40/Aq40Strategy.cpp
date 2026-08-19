#include "Aq40Strategy.h"

#include "Aq40Multipliers.h"
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
    triggers.push_back(
        new TriggerNode("aq40 twins tank pickup", { NextAction("aq40 twins tank pickup", ACTION_RAID) }));
    triggers.push_back(
        new TriggerNode("aq40 twins separate", { NextAction("aq40 twins separate", ACTION_RAID) }));
    triggers.push_back(
        new TriggerNode("aq40 twins caster range", { NextAction("aq40 twins caster range", ACTION_RAID) }));

    // Positioning batch 1: rear flanks (frontal cones/sweeps), ground-effect
    // step-outs, Yauj fear counters, Ouro mound kiting.
    triggers.push_back(new TriggerNode("aq40 ouro flank", { NextAction("rear flank", ACTION_MOVE + 4) }));
    triggers.push_back(new TriggerNode("aq40 sartura flank", { NextAction("rear flank", ACTION_MOVE + 4) }));
    for (char const* ground : { "aq40 kri cloud", "aq40 viscidus toxin", "aq40 huhuran poison", "aq40 twins blizzard" })
        triggers.push_back(new TriggerNode(ground, { NextAction("aq40 move from ground effect", ACTION_RAID) }));
    triggers.push_back(new TriggerNode("aq40 yauj tremor", { NextAction("tremor totem", ACTION_RAID) }));
    triggers.push_back(new TriggerNode("aq40 yauj fear ward", { NextAction("aq40 fear ward", ACTION_RAID) }));
    triggers.push_back(new TriggerNode("aq40 ouro mound", { NextAction("aq40 flee mound", ACTION_RAID) }));

    // Ranged/healer standoff bands: Huhuran's bolts hit the nearest ~15;
    // Skeram's Arcane Explosion is a big point-blank AoE.
    triggers.push_back(new TriggerNode("aq40 huhuran standoff", { NextAction("aq40 huhuran standoff", ACTION_RAID) }));
    triggers.push_back(new TriggerNode("aq40 skeram standoff", { NextAction("aq40 skeram standoff", ACTION_RAID) }));
    triggers.push_back(new TriggerNode("aq40 giant claw sitter", { NextAction("aq40 giant claw sitter", ACTION_RAID) }));
    // Vem's knockback strips 80% threat; Ouro's Sand Blast wipes the tank's.
    triggers.push_back(new TriggerNode("aq40 skeram tank pickup", { NextAction("aq40 skeram tank pickup", ACTION_RAID + 1) }));
    triggers.push_back(new TriggerNode("aq40 skeram healer follow", { NextAction("aq40 skeram healer follow", ACTION_RAID + 1) }));
    triggers.push_back(new TriggerNode("aq40 vem backup taunt", { NextAction("aq40 vem backup taunt", ACTION_RAID + 1) }));
    triggers.push_back(new TriggerNode("aq40 ouro backup taunt", { NextAction("aq40 ouro backup taunt", ACTION_RAID + 1) }));
}

void RaidAq40Strategy::InitMultipliers(std::vector<Multiplier*>& multipliers)
{
    multipliers.push_back(new Aq40TwinsDutyMultiplier(botAI));
}

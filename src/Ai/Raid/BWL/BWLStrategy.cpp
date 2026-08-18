#include "BWLStrategy.h"

#include "BWLMultipliers.h"
#include "RaidBossScripts.h"

void RaidBwlStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    // Shadow Flame breath / cleave / tail-sweep bosses: everyone but the
    // tank holds the rear flank.
    for (char const* flank : { "bwl firemaw flank", "bwl ebonroc flank", "bwl flamegor flank",
                               "bwl nefarian flank", "bwl broodlord flank" })
        triggers.push_back(new TriggerNode(flank, { NextAction("rear flank", ACTION_MOVE + 4) }));

    triggers.push_back(new TriggerNode("bwl broodlord standoff", { NextAction("bwl broodlord standoff", ACTION_RAID) }));

    triggers.push_back(new TriggerNode("often", {
        NextAction("bwl check onyxia scale cloak", ACTION_RAID) }));
    triggers.push_back(new TriggerNode("bwl suppression device", {
        NextAction("bwl turn off suppression device", ACTION_RAID) }));

    triggers.push_back(new TriggerNode("bwl razorgore fire resistance", {
        NextAction("bwl razorgore fire resistance", ACTION_RAID) }));
    triggers.push_back(new TriggerNode("bwl razorgore not mind controlled", {
        NextAction("bwl razorgore avoid aoe", ACTION_RAID) }));
    triggers.push_back(new TriggerNode("bwl razorgore not mind controlled", {
        NextAction("bwl razorgore mark boss", ACTION_RAID + 1) }));

    triggers.push_back(new TriggerNode("bwl vaelastrasz fire resistance", {
        NextAction("bwl vaelastrasz fire resistance", ACTION_RAID) }));
    triggers.push_back(new TriggerNode("bwl vaelastrasz positioning", {
        NextAction("rear flank", ACTION_MOVE + 4) }));
    triggers.push_back(new TriggerNode("bwl vaelastrasz burning adrenaline", {
        NextAction("bwl vaelastrasz move away", ACTION_RAID + 5) }));

    // Ebonroc: the elected clean tank taunts him off the shadowed one.
    triggers.push_back(new TriggerNode("bwl ebonroc shadow swap", {
        NextAction("bwl ebonroc taunt", ACTION_RAID) }));

    // Nefarian: tremor vs Bellowing Roar; phase-1 drakonids get a focused
    // kill order (chromatics first) while Nefarian is still on the balcony.
    triggers.push_back(new TriggerNode("bwl nefarian tremor totem", {
        NextAction("tremor totem", ACTION_RAID) }));
    triggers.push_back(new TriggerNode("bwl nefarian drakonids", {
        NextAction("bwl nefarian drakonid mark", ACTION_RAID) }));

    // Flamegor and Chromaggus both frenzy; hunters tranq at raid priority
    // the moment it is up (same pattern as Magmadar).
    triggers.push_back(new TriggerNode("bwl flamegor frenzy", {
        NextAction("tranquilizing shot", ACTION_RAID) }));
    triggers.push_back(new TriggerNode("bwl chromaggus frenzy", {
        NextAction("tranquilizing shot", ACTION_RAID) }));

    triggers.push_back(new TriggerNode("bwl affliction bronze", {
        NextAction("bwl use hourglass sand", ACTION_RAID) }));

    triggers.push_back(new TriggerNode("bwl wild magic", {
        NextAction("ice block", ACTION_RAID) }));
    triggers.push_back(new TriggerNode("bwl nefarian fear ward", {
        NextAction("bwl nefarian fear ward", ACTION_RAID) }));

    triggers.push_back(new TriggerNode("bwl death talon wyrmguard tank", {
        NextAction("bwl death talon wyrmguard tank move away", ACTION_RAID) }));
    triggers.push_back(new TriggerNode("bwl death talon wyrmguard ranged", {
        NextAction("bwl death talon wyrmguard ranged move away", ACTION_RAID) }));
}

void RaidBwlStrategy::InitMultipliers(std::vector<Multiplier*>& multipliers)
{
    multipliers.push_back(new RazorgoreTankMultiplier(botAI));
    multipliers.push_back(new VaelastraszTankMultiplier(botAI));
    multipliers.push_back(new VaelastraszBurningAdrenalineMultiplier(botAI));
    // Chromaggus' brood afflictions (curse/magic/poison/disease) stack up
    // faster than casual dispelling clears them.
    multipliers.push_back(new RaidDispelUrgencyMultiplier(botAI, "bwl dispel urgency multiplier", { "chromaggus" }));
}

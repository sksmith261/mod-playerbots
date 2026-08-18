#include "MCStrategy.h"

#include "MCMultipliers.h"
#include "RaidBossScripts.h"
#include "Strategy.h"

void RaidMcStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    // Ranged/healers stay out of Panic range (and off the moat lip).
    triggers.push_back(new TriggerNode("mc magmadar standoff", { NextAction("mc magmadar standoff", ACTION_RAID) }));
    triggers.push_back(new TriggerNode("mc geddon armageddon", { NextAction("mc geddon armageddon", ACTION_RAID + 1) }));
    triggers.push_back(new TriggerNode("mc ragnaros tank reentry", { NextAction("mc ragnaros tank reentry", ACTION_RAID) }));

    // Lucifron
    triggers.push_back(
        new TriggerNode("mc lucifron shadow resistance",
                        { NextAction("mc lucifron shadow resistance", ACTION_RAID) }));
    triggers.push_back(
        new TriggerNode("mc lucifron mark",
                        { NextAction("mc lucifron mark", ACTION_RAID) }));

    // Magmadar
    triggers.push_back(
        new TriggerNode("mc magmadar fire resistance",
                        { NextAction("mc magmadar fire resistance", ACTION_RAID) }));
    // Hunters keep the generic enrage-tranq at rotation priority; this raises
    // it to raid priority the moment Frenzy is actually up.
    triggers.push_back(
        new TriggerNode("mc magmadar frenzy",
                        { NextAction("tranquilizing shot", ACTION_RAID) }));
    triggers.push_back(
        new TriggerNode("mc magmadar tremor totem",
                        { NextAction("tremor totem", ACTION_RAID) }));
    triggers.push_back(
        new TriggerNode("mc magmadar fear ward",
                        { NextAction("mc magmadar fear ward", ACTION_RAID) }));
    triggers.push_back(
        new TriggerNode("mc magmadar lava bomb",
                        { NextAction("mc move from ground effect", ACTION_RAID) }));

    // Gehennas
    triggers.push_back(
        new TriggerNode("mc gehennas shadow resistance",
                        { NextAction("mc gehennas shadow resistance", ACTION_RAID) }));
    triggers.push_back(
        new TriggerNode("mc gehennas mark",
                        { NextAction("mc gehennas mark", ACTION_RAID) }));
    triggers.push_back(
        new TriggerNode("mc gehennas rain of fire",
                        { NextAction("mc move from ground effect", ACTION_RAID) }));

    // Garr
    triggers.push_back(
        new TriggerNode("mc garr fire resistance",
                        { NextAction("mc garr fire resistance", ACTION_RAID) }));
    triggers.push_back(
        new TriggerNode("mc garr banish",
                        { NextAction("mc garr banish", ACTION_RAID) }));
    triggers.push_back(
        new TriggerNode("mc garr mark",
                        { NextAction("mc garr mark", ACTION_RAID) }));

    // Baron Geddon
    triggers.push_back(
        new TriggerNode("mc baron geddon fire resistance",
                        { NextAction("mc baron geddon fire resistance", ACTION_RAID) }));
    triggers.push_back(
        new TriggerNode("mc living bomb debuff",
                        { NextAction("mc move from group", ACTION_RAID) }));
    triggers.push_back(
        new TriggerNode("mc baron geddon inferno",
                        { NextAction("mc move from baron geddon", ACTION_RAID) }));

    // Shazzrah
    triggers.push_back(
        new TriggerNode("mc shazzrah ranged",
                        { NextAction("mc shazzrah move away", ACTION_RAID) }));
    triggers.push_back(
        new TriggerNode("mc shazzrah purge",
                        { NextAction("mc shazzrah purge", ACTION_RAID) }));

    // Sulfuron Harbinger
    // Alternatively, shadow resistance is also possible.
    triggers.push_back(
        new TriggerNode("mc sulfuron harbinger fire resistance",
                        { NextAction("mc sulfuron harbinger fire resistance", ACTION_RAID) }));
    // Focusing a Flamewaker Priest also concentrates the raid's generic
    // interrupt triggers on it, shutting down Dark Mending.
    triggers.push_back(
        new TriggerNode("mc sulfuron mark",
                        { NextAction("mc sulfuron mark", ACTION_RAID) }));

    // Golemagg the Incinerator
    triggers.push_back(
        new TriggerNode("mc golemagg fire resistance",
                        { NextAction("mc golemagg fire resistance", ACTION_RAID) }));
    triggers.push_back(
        new TriggerNode("mc golemagg mark boss",
                        { NextAction("mc golemagg mark boss", ACTION_RAID) }));
    triggers.push_back(
        new TriggerNode("mc golemagg is main tank",
                        { NextAction("mc golemagg main tank attack golemagg", ACTION_RAID) }));
    triggers.push_back(
        new TriggerNode("mc golemagg is assist tank",
                        { NextAction("mc golemagg assist tank attack core rager", ACTION_RAID) }));

    // Majordomo Executus
    triggers.push_back(
        new TriggerNode("mc majordomo shadow resistance",
                        { NextAction("mc majordomo shadow resistance", ACTION_RAID) }));
    // Healers first, then elites; prefers (and switches to) adds without a
    // reflection shield. The boss himself just despawns his aegis when the
    // adds are done, so the mark tiers are the whole fight.
    triggers.push_back(
        new TriggerNode("mc majordomo mark",
                        { NextAction("mc majordomo mark", ACTION_RAID) }));

    // Ragnaros
    triggers.push_back(
        new TriggerNode("mc ragnaros fire resistance",
                        { NextAction("mc ragnaros fire resistance", ACTION_RAID) }));
    // Submerge phase: focus the Sons of Flame down one at a time so they die
    // before the 90s emerge timer. AoE stays allowed here — it's the one MC
    // fight where it helps.
    triggers.push_back(
        new TriggerNode("mc ragnaros sons",
                        { NextAction("mc ragnaros sons mark", ACTION_RAID) }));

    // Trash
    triggers.push_back(
        new TriggerNode("mc core hound mark",
                        { NextAction("mc core hound mark", ACTION_RAID) }));
}

void RaidMcStrategy::InitMultipliers(std::vector<Multiplier*>& multipliers)
{
    multipliers.push_back(new RaidDispelUrgencyMultiplier(botAI, "mc dispel urgency multiplier", { "lucifron", "shazzrah", "gehennas" }));
    multipliers.push_back(new MajordomoReflectionMultiplier(botAI));
    multipliers.push_back(new GarrDisableDpsAoeMultiplier(botAI));
    multipliers.push_back(new BaronGeddonAbilityMultiplier(botAI));
    multipliers.push_back(new GolemaggMultiplier(botAI));
}

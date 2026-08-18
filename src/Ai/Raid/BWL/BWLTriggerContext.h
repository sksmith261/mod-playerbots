#ifndef PLAYERBOTS_BWLTRIGGERCONTEXT_H
#define PLAYERBOTS_BWLTRIGGERCONTEXT_H

#include "NamedObjectContext.h"
#include "BWLHelpers.h"
#include "BWLTriggers.h"
#include "RaidBossScripts.h"

class RaidBwlTriggerContext : public NamedObjectContext<Trigger>
{
public:
    RaidBwlTriggerContext()
    {
        creators["bwl suppression device"] = &RaidBwlTriggerContext::bwl_suppression_device;

        creators["bwl razorgore fire resistance"] = &RaidBwlTriggerContext::bwl_razorgore_fire_resistance_trigger;
        creators["bwl razorgore not mind controlled"] = &RaidBwlTriggerContext::bwl_razorgore_not_mind_controlled;

        creators["bwl vaelastrasz fire resistance"] = &RaidBwlTriggerContext::bwl_vaelastrasz_fire_resistance_trigger;
        creators["bwl vaelastrasz positioning"] = &RaidBwlTriggerContext::bwl_vaelastrasz_positioning;
        creators["bwl vaelastrasz burning adrenaline"] = &RaidBwlTriggerContext::bwl_vaelastrasz_burning_adrenaline;

        creators["bwl ebonroc shadow swap"] = &RaidBwlTriggerContext::bwl_ebonroc_shadow_swap;
        creators["bwl nefarian tremor totem"] = &RaidBwlTriggerContext::bwl_nefarian_tremor_totem;
        creators["bwl nefarian drakonids"] = &RaidBwlTriggerContext::bwl_nefarian_drakonids;
        creators["bwl firemaw flank"] = &RaidBwlTriggerContext::bwl_firemaw_flank;
        creators["bwl ebonroc flank"] = &RaidBwlTriggerContext::bwl_ebonroc_flank;
        creators["bwl flamegor flank"] = &RaidBwlTriggerContext::bwl_flamegor_flank;
        creators["bwl nefarian flank"] = &RaidBwlTriggerContext::bwl_nefarian_flank;
        creators["bwl broodlord flank"] = &RaidBwlTriggerContext::bwl_broodlord_flank;
        creators["bwl broodlord standoff"] = &RaidBwlTriggerContext::bwl_broodlord_standoff;
        creators["bwl flamegor frenzy"] = &RaidBwlTriggerContext::bwl_flamegor_frenzy;
        creators["bwl chromaggus frenzy"] = &RaidBwlTriggerContext::bwl_chromaggus_frenzy;
        creators["bwl affliction bronze"] = &RaidBwlTriggerContext::bwl_affliction_bronze;
        creators["bwl wild magic"] = &RaidBwlTriggerContext::bwl_wild_magic;
        creators["bwl nefarian fear ward"] = &RaidBwlTriggerContext::bwl_nefarian_fear_ward;
        creators["bwl death talon wyrmguard tank"] = &RaidBwlTriggerContext::bwl_death_talon_wyrmguard_tank;
        creators["bwl death talon wyrmguard ranged"] = &RaidBwlTriggerContext::bwl_death_talon_wyrmguard_ranged;
    }

private:
    static Trigger* bwl_ebonroc_shadow_swap(PlayerbotAI* botAI) { return new BwlEbonrocShadowSwapTrigger(botAI); }
    static Trigger* bwl_nefarian_tremor_totem(PlayerbotAI* botAI) { return new RaidTremorTotemTrigger(botAI, "bwl nefarian tremor totem", "nefarian"); }
    static Trigger* bwl_nefarian_drakonids(PlayerbotAI* botAI)
    {
        using namespace BlackwingLairHelpers;
        return new RaidAddsAliveMarkTrigger(botAI, "bwl nefarian drakonids", NEFARIAN_DRAKONIDS);
    }
    static Trigger* bwl_firemaw_flank(PlayerbotAI* ai) { return new RaidRearFlankTrigger(ai, "bwl firemaw flank", "firemaw"); }
    static Trigger* bwl_ebonroc_flank(PlayerbotAI* ai) { return new RaidRearFlankTrigger(ai, "bwl ebonroc flank", "ebonroc"); }
    static Trigger* bwl_flamegor_flank(PlayerbotAI* ai) { return new RaidRearFlankTrigger(ai, "bwl flamegor flank", "flamegor"); }
    static Trigger* bwl_nefarian_flank(PlayerbotAI* ai) { return new RaidRearFlankTrigger(ai, "bwl nefarian flank", "nefarian"); }
    static Trigger* bwl_broodlord_flank(PlayerbotAI* ai) { return new RaidRearFlankTrigger(ai, "bwl broodlord flank", "broodlord lashlayer"); }
    static Trigger* bwl_broodlord_standoff(PlayerbotAI* ai)
    { return new RaidStandoffTrigger(ai, "bwl broodlord standoff", "broodlord lashlayer", 22.0f); }
    static Trigger* bwl_flamegor_frenzy(PlayerbotAI* botAI)
    {
        return new RaidFrenzyTranqTrigger(botAI, "bwl flamegor frenzy", "flamegor",
            static_cast<uint32>(BlackwingLairHelpers::BlackwingLairSpells::SPELL_FLAMEGOR_FRENZY));
    }
    static Trigger* bwl_chromaggus_frenzy(PlayerbotAI* botAI)
    {
        return new RaidFrenzyTranqTrigger(botAI, "bwl chromaggus frenzy", "chromaggus",
            static_cast<uint32>(BlackwingLairHelpers::BlackwingLairSpells::SPELL_CHROMAGGUS_FRENZY));
    }

    static Trigger* bwl_suppression_device(PlayerbotAI* ai) { return new BwlSuppressionDeviceTrigger(ai); }
    static Trigger* bwl_razorgore_fire_resistance_trigger(PlayerbotAI* ai) { return new BossFireResistanceTrigger(ai, "razorgore the untamed"); }
    static Trigger* bwl_razorgore_not_mind_controlled(PlayerbotAI* ai) { return new BwlRazorgoreNotMindControlledTrigger(ai); }
    static Trigger* bwl_vaelastrasz_fire_resistance_trigger(PlayerbotAI* ai) { return new BossFireResistanceTrigger(ai, "vaelastrasz the corrupt"); }
    static Trigger* bwl_vaelastrasz_positioning(PlayerbotAI* ai) { return new BwlVaelastraszPositioningTrigger(ai); }
    static Trigger* bwl_vaelastrasz_burning_adrenaline(PlayerbotAI* ai) { return new BwlVaelastraszBurningAdrenalineTrigger(ai); }
    static Trigger* bwl_affliction_bronze(PlayerbotAI* ai) { return new BwlAfflictionBronzeTrigger(ai); }
    static Trigger* bwl_wild_magic(PlayerbotAI* ai) { return new BwlWildMagicTrigger(ai); }
    static Trigger* bwl_nefarian_fear_ward(PlayerbotAI* ai) { return new BwlNefarianFearWardTrigger(ai); }
    static Trigger* bwl_death_talon_wyrmguard_tank(PlayerbotAI* ai) { return new BwlDeathTalonWyrmguardTankTrigger(ai); }
    static Trigger* bwl_death_talon_wyrmguard_ranged(PlayerbotAI* ai) { return new BwlDeathTalonWyrmguardRangedTrigger(ai); }
};

#endif

#ifndef PLAYERBOTS_AQ40TRIGGERCONTEXT_H
#define PLAYERBOTS_AQ40TRIGGERCONTEXT_H

#include "NamedObjectContext.h"
#include "Aq40Triggers.h"
#include "Aq40Utils.h"

class RaidAq40TriggerContext : public NamedObjectContext<Trigger>
{
public:
    RaidAq40TriggerContext()
    {
        creators["aq40 in stomach"] = &RaidAq40TriggerContext::in_stomach;
        creators["aq40 dark glare"] = &RaidAq40TriggerContext::dark_glare;
        creators["aq40 skeram mark"] = &RaidAq40TriggerContext::skeram_mark;
        creators["aq40 sartura whirlwind"] = &RaidAq40TriggerContext::sartura_whirlwind;
        creators["aq40 sartura mark"] = &RaidAq40TriggerContext::sartura_mark;
        creators["aq40 bug trio"] = &RaidAq40TriggerContext::bug_trio;
        creators["aq40 fankriss worms"] = &RaidAq40TriggerContext::fankriss_worms;
        creators["aq40 huhuran frenzy"] = &RaidAq40TriggerContext::huhuran_frenzy;
        creators["aq40 twins wrong target"] = &RaidAq40TriggerContext::twins_wrong_target;
        creators["aq40 twins tank pickup"] = &RaidAq40TriggerContext::twins_tank_pickup;
        creators["aq40 twins separate"] = &RaidAq40TriggerContext::twins_separate;
        creators["aq40 twins caster range"] = &RaidAq40TriggerContext::twins_caster_range;
        creators["aq40 ouro flank"] = &RaidAq40TriggerContext::ouro_flank;
        creators["aq40 sartura flank"] = &RaidAq40TriggerContext::sartura_flank;
        creators["aq40 kri cloud"] = &RaidAq40TriggerContext::kri_cloud;
        creators["aq40 viscidus toxin"] = &RaidAq40TriggerContext::viscidus_toxin;
        creators["aq40 huhuran poison"] = &RaidAq40TriggerContext::huhuran_poison;
        creators["aq40 twins blizzard"] = &RaidAq40TriggerContext::twins_blizzard;
        creators["aq40 yauj tremor"] = &RaidAq40TriggerContext::yauj_tremor;
        creators["aq40 yauj fear ward"] = &RaidAq40TriggerContext::yauj_fear_ward;
        creators["aq40 ouro mound"] = &RaidAq40TriggerContext::ouro_mound;
        creators["aq40 huhuran standoff"] = &RaidAq40TriggerContext::huhuran_standoff;
        creators["aq40 skeram standoff"] = &RaidAq40TriggerContext::skeram_standoff;
        creators["aq40 giant claw sitter"] = &RaidAq40TriggerContext::giant_claw_sitter;
    }

private:
    static Trigger* in_stomach(PlayerbotAI* ai) { return new Aq40InStomachTrigger(ai); }
    static Trigger* dark_glare(PlayerbotAI* ai) { return new Aq40DarkGlareTrigger(ai); }
    static Trigger* skeram_mark(PlayerbotAI* ai) { return new Aq40SkeramMarkTrigger(ai); }
    static Trigger* sartura_whirlwind(PlayerbotAI* ai) { return new Aq40SarturaWhirlwindTrigger(ai); }
    static Trigger* sartura_mark(PlayerbotAI* ai)
    { return new RaidKillOrderMarkTrigger(ai, "aq40 sartura mark", "battleguard sartura"); }
    static Trigger* bug_trio(PlayerbotAI* ai)
    {
        using namespace RaidAq40;
        return new RaidAddsAliveMarkTrigger(ai, "aq40 bug trio", { NPC_PRINCESS_YAUJ, NPC_LORD_KRI, NPC_VEM });
    }
    static Trigger* fankriss_worms(PlayerbotAI* ai)
    { return new RaidAddsAliveMarkTrigger(ai, "aq40 fankriss worms",
        { RaidAq40::NPC_VEKNISS_HATCHLING, RaidAq40::NPC_SPAWN_OF_FANKRISS }); }
    static Trigger* huhuran_frenzy(PlayerbotAI* ai)
    { return new RaidFrenzyTranqTrigger(ai, "aq40 huhuran frenzy", "princess huhuran", RaidAq40::SPELL_HUHURAN_FRENZY); }
    static Trigger* twins_wrong_target(PlayerbotAI* ai) { return new Aq40TwinsWrongTargetTrigger(ai); }
    static Trigger* twins_tank_pickup(PlayerbotAI* ai) { return new Aq40TwinsTankPickupTrigger(ai); }
    static Trigger* twins_separate(PlayerbotAI* ai) { return new Aq40TwinsSeparateTrigger(ai); }
    static Trigger* twins_caster_range(PlayerbotAI* ai) { return new Aq40TwinsCasterRangeTrigger(ai); }
    static Trigger* ouro_flank(PlayerbotAI* ai) { return new RaidRearFlankTrigger(ai, "aq40 ouro flank", "ouro"); }
    static Trigger* sartura_flank(PlayerbotAI* ai) { return new RaidRearFlankTrigger(ai, "aq40 sartura flank", "battleguard sartura"); }
    static Trigger* kri_cloud(PlayerbotAI* ai) { return new RaidGroundEffectAuraTrigger(ai, "aq40 kri cloud", RaidAq40::SPELL_KRI_POISON_CLOUD); }
    static Trigger* viscidus_toxin(PlayerbotAI* ai) { return new RaidGroundEffectAuraTrigger(ai, "aq40 viscidus toxin", RaidAq40::SPELL_VISCIDUS_TOXIN); }
    static Trigger* huhuran_poison(PlayerbotAI* ai) { return new RaidGroundEffectAuraTrigger(ai, "aq40 huhuran poison", RaidAq40::SPELL_HUHURAN_NOXIOUS_POISON); }
    static Trigger* twins_blizzard(PlayerbotAI* ai) { return new RaidGroundEffectAuraTrigger(ai, "aq40 twins blizzard", RaidAq40::SPELL_VEKLOR_BLIZZARD); }
    static Trigger* yauj_tremor(PlayerbotAI* ai) { return new RaidTremorTotemTrigger(ai, "aq40 yauj tremor", "princess yauj"); }
    static Trigger* yauj_fear_ward(PlayerbotAI* ai) { return new RaidFearWardTrigger(ai, "aq40 yauj fear ward", "princess yauj"); }
    static Trigger* ouro_mound(PlayerbotAI* ai) { return new Aq40OuroMoundTrigger(ai); }
    static Trigger* giant_claw_sitter(PlayerbotAI* ai) { return new Aq40GiantClawSitterTrigger(ai); }
    static Trigger* huhuran_standoff(PlayerbotAI* ai)
    { return new RaidStandoffTrigger(ai, "aq40 huhuran standoff", "princess huhuran", 30.0f); }
    static Trigger* skeram_standoff(PlayerbotAI* ai)
    { return new RaidStandoffTrigger(ai, "aq40 skeram standoff", "the prophet skeram", 22.0f); }
};

#endif

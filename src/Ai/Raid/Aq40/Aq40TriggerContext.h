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
    { return new RaidAddsAliveMarkTrigger(ai, "aq40 fankriss worms", { RaidAq40::NPC_SPAWN_OF_FANKRISS }); }
    static Trigger* huhuran_frenzy(PlayerbotAI* ai)
    { return new RaidFrenzyTranqTrigger(ai, "aq40 huhuran frenzy", "princess huhuran", RaidAq40::SPELL_HUHURAN_FRENZY); }
    static Trigger* twins_wrong_target(PlayerbotAI* ai) { return new Aq40TwinsWrongTargetTrigger(ai); }
    static Trigger* twins_tank_pickup(PlayerbotAI* ai) { return new Aq40TwinsTankPickupTrigger(ai); }
    static Trigger* twins_separate(PlayerbotAI* ai) { return new Aq40TwinsSeparateTrigger(ai); }
    static Trigger* twins_caster_range(PlayerbotAI* ai) { return new Aq40TwinsCasterRangeTrigger(ai); }
};

#endif

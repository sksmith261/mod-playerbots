#ifndef PLAYERBOTS_MCTRIGGERCONTEXT_H
#define PLAYERBOTS_MCTRIGGERCONTEXT_H

#include "BossAuraTriggers.h"
#include "NamedObjectContext.h"
#include "MCTriggers.h"

class RaidMcTriggerContext : public NamedObjectContext<Trigger>
{
public:
    RaidMcTriggerContext()
    {
        creators["mc lucifron shadow resistance"] = &RaidMcTriggerContext::lucifron_shadow_resistance;
        creators["mc magmadar fire resistance"] = &RaidMcTriggerContext::magmadar_fire_resistance;
        creators["mc gehennas shadow resistance"] = &RaidMcTriggerContext::gehennas_shadow_resistance;
        creators["mc garr fire resistance"] = &RaidMcTriggerContext::garr_fire_resistance;
        creators["mc baron geddon fire resistance"] = &RaidMcTriggerContext::baron_geddon_fire_resistance;
        creators["mc living bomb debuff"] = &RaidMcTriggerContext::living_bomb_debuff;
        creators["mc baron geddon inferno"] = &RaidMcTriggerContext::baron_geddon_inferno;
        creators["mc shazzrah ranged"] = &RaidMcTriggerContext::shazzrah_ranged;
        creators["mc sulfuron harbinger fire resistance"] = &RaidMcTriggerContext::sulfuron_harbinger_fire_resistance;
        creators["mc golemagg fire resistance"] = &RaidMcTriggerContext::golemagg_fire_resistance;
        creators["mc golemagg mark boss"] = &RaidMcTriggerContext::golemagg_mark_boss;
        creators["mc golemagg is main tank"] = &RaidMcTriggerContext::golemagg_is_main_tank;
        creators["mc golemagg is assist tank"] = &RaidMcTriggerContext::golemagg_is_assist_tank;
        creators["mc majordomo shadow resistance"] = &RaidMcTriggerContext::majordomo_shadow_resistance;
        creators["mc ragnaros fire resistance"] = &RaidMcTriggerContext::ragnaros_fire_resistance;
        creators["mc core hound mark"] = &RaidMcTriggerContext::core_hound_mark;
        creators["mc lucifron mark"] = &RaidMcTriggerContext::lucifron_mark;
        creators["mc magmadar frenzy"] = &RaidMcTriggerContext::magmadar_frenzy;
        creators["mc magmadar tremor totem"] = &RaidMcTriggerContext::magmadar_tremor_totem;
        creators["mc magmadar fear ward"] = &RaidMcTriggerContext::magmadar_fear_ward;
        creators["mc magmadar lava bomb"] = &RaidMcTriggerContext::magmadar_lava_bomb;
        creators["mc garr banish"] = &RaidMcTriggerContext::garr_banish;
        creators["mc garr mark"] = &RaidMcTriggerContext::garr_mark;
        creators["mc shazzrah purge"] = &RaidMcTriggerContext::shazzrah_purge;
    }

private:
    static Trigger* lucifron_shadow_resistance(PlayerbotAI* botAI) { return new BossShadowResistanceTrigger(botAI, "lucifron"); }
    static Trigger* lucifron_mark(PlayerbotAI* botAI) { return new McLucifronMarkTrigger(botAI); }
    static Trigger* magmadar_frenzy(PlayerbotAI* botAI) { return new McMagmadarFrenzyTrigger(botAI); }
    static Trigger* magmadar_tremor_totem(PlayerbotAI* botAI) { return new McMagmadarTremorTotemTrigger(botAI); }
    static Trigger* magmadar_fear_ward(PlayerbotAI* botAI) { return new McMagmadarFearWardTrigger(botAI); }
    static Trigger* magmadar_lava_bomb(PlayerbotAI* botAI) { return new McMagmadarLavaBombTrigger(botAI); }
    static Trigger* garr_banish(PlayerbotAI* botAI) { return new McGarrBanishTrigger(botAI); }
    static Trigger* garr_mark(PlayerbotAI* botAI) { return new McGarrMarkTrigger(botAI); }
    static Trigger* shazzrah_purge(PlayerbotAI* botAI) { return new McShazzrahPurgeTrigger(botAI); }
    static Trigger* magmadar_fire_resistance(PlayerbotAI* botAI) { return new BossFireResistanceTrigger(botAI, "magmadar"); }
    static Trigger* gehennas_shadow_resistance(PlayerbotAI* botAI) { return new BossShadowResistanceTrigger(botAI, "gehennas"); }
    static Trigger* garr_fire_resistance(PlayerbotAI* botAI) { return new BossFireResistanceTrigger(botAI, "garr"); }
    static Trigger* baron_geddon_fire_resistance(PlayerbotAI* botAI) { return new BossFireResistanceTrigger(botAI, "baron geddon"); }
    static Trigger* living_bomb_debuff(PlayerbotAI* botAI) { return new McLivingBombDebuffTrigger(botAI); }
    static Trigger* baron_geddon_inferno(PlayerbotAI* botAI) { return new McBaronGeddonInfernoTrigger(botAI); }
    static Trigger* shazzrah_ranged(PlayerbotAI* botAI) { return new McShazzrahRangedTrigger(botAI); }
    static Trigger* sulfuron_harbinger_fire_resistance(PlayerbotAI* botAI) { return new BossFireResistanceTrigger(botAI, "sulfuron harbinger"); }
    static Trigger* golemagg_fire_resistance(PlayerbotAI* botAI) { return new BossFireResistanceTrigger(botAI, "golemagg the incinerator"); }
    static Trigger* golemagg_mark_boss(PlayerbotAI* botAI) { return new McGolemaggMarkBossTrigger(botAI); }
    static Trigger* golemagg_is_main_tank(PlayerbotAI* botAI) { return new McGolemaggIsMainTankTrigger(botAI); }
    static Trigger* golemagg_is_assist_tank(PlayerbotAI* botAI) { return new McGolemaggIsAssistTankTrigger(botAI); }
    static Trigger* majordomo_shadow_resistance(PlayerbotAI* botAI) { return new BossShadowResistanceTrigger(botAI, "majordomo executus"); }
    static Trigger* ragnaros_fire_resistance(PlayerbotAI* botAI) { return new BossFireResistanceTrigger(botAI, "ragnaros"); }
    static Trigger* core_hound_mark(PlayerbotAI* botAI) { return new McCoreHoundMarkTrigger(botAI); }
};

#endif

#ifndef PLAYERBOTS_BWLACTIONCONTEXT_H
#define PLAYERBOTS_BWLACTIONCONTEXT_H

#include "Action.h"
#include "NamedObjectContext.h"
#include "BWLActions.h"
#include "BWLHelpers.h"
#include "RaidBossScripts.h"

class RaidBwlActionContext : public NamedObjectContext<Action>
{
public:
    RaidBwlActionContext()
    {
        creators["bwl broodlord standoff"] = &RaidBwlActionContext::bwl_broodlord_standoff;
        creators["bwl broodlord backup taunt"] = &RaidBwlActionContext::bwl_broodlord_taunt;
        creators["bwl chromaggus backup taunt"] = &RaidBwlActionContext::bwl_chromaggus_taunt;
        creators["bwl nefarian door camp"] = &RaidBwlActionContext::bwl_nefarian_door_camp;
        creators["bwl firemaw backup taunt"] = &RaidBwlActionContext::bwl_firemaw_taunt;
        creators["bwl flamegor backup taunt"] = &RaidBwlActionContext::bwl_flamegor_taunt;
        creators["bwl broodlord reentry"] = &RaidBwlActionContext::bwl_broodlord_reentry;
        creators["bwl firemaw reentry"] = &RaidBwlActionContext::bwl_firemaw_reentry;
        creators["bwl flamegor reentry"] = &RaidBwlActionContext::bwl_flamegor_reentry;
        creators["bwl ebonroc taunt"] = &RaidBwlActionContext::bwl_ebonroc_taunt;
        creators["bwl nefarian drakonid mark"] = &RaidBwlActionContext::bwl_nefarian_drakonid_mark;
        creators["bwl check onyxia scale cloak"] = &RaidBwlActionContext::bwl_check_onyxia_scale_cloak;
        creators["bwl turn off suppression device"] = &RaidBwlActionContext::bwl_turn_off_suppression_device;

        creators["bwl razorgore fire resistance"] = &RaidBwlActionContext::bwl_razorgore_fire_resistance_action;
        creators["bwl razorgore avoid aoe"] = &RaidBwlActionContext::bwl_razorgore_avoid_aoe;
        creators["bwl razorgore mark boss"] = &RaidBwlActionContext::bwl_razorgore_mark_boss;

        creators["bwl vaelastrasz fire resistance"] = &RaidBwlActionContext::bwl_vaelastrasz_fire_resistance_action;
        creators["bwl vaelastrasz move away"] = &RaidBwlActionContext::bwl_vaelastrasz_move_away;

        creators["bwl use hourglass sand"] = &RaidBwlActionContext::bwl_use_hourglass_sand;
        creators["bwl nefarian fear ward"] = &RaidBwlActionContext::bwl_nefarian_fear_ward;
        creators["bwl death talon wyrmguard tank move away"] = &RaidBwlActionContext::bwl_death_talon_wyrmguard_tank_move_away;
        creators["bwl death talon wyrmguard ranged move away"] = &RaidBwlActionContext::bwl_death_talon_wyrmguard_ranged_move_away;
    }

private:
    static Action* bwl_chromaggus_taunt(PlayerbotAI* ai)
    { return new RaidBackupTauntAction(ai, "bwl chromaggus backup taunt", "chromaggus"); }
    static Action* bwl_nefarian_door_camp(PlayerbotAI* ai) { return new BwlNefarianDoorCampAction(ai); }
    static Action* bwl_broodlord_taunt(PlayerbotAI* ai)
    { return new RaidBackupTauntAction(ai, "bwl broodlord backup taunt", "broodlord lashlayer"); }
    static Action* bwl_firemaw_taunt(PlayerbotAI* ai)
    { return new RaidBackupTauntAction(ai, "bwl firemaw backup taunt", "firemaw"); }
    static Action* bwl_flamegor_taunt(PlayerbotAI* ai)
    { return new RaidBackupTauntAction(ai, "bwl flamegor backup taunt", "flamegor"); }
    static Action* bwl_broodlord_reentry(PlayerbotAI* ai)
    { return new RaidTankReentryAction(ai, "bwl broodlord reentry", "broodlord lashlayer"); }
    static Action* bwl_firemaw_reentry(PlayerbotAI* ai)
    { return new RaidTankReentryAction(ai, "bwl firemaw reentry", "firemaw"); }
    static Action* bwl_flamegor_reentry(PlayerbotAI* ai)
    { return new RaidTankReentryAction(ai, "bwl flamegor reentry", "flamegor"); }
    static Action* bwl_broodlord_standoff(PlayerbotAI* ai)
    { return new RaidStandoffAction(ai, "bwl broodlord standoff", "broodlord lashlayer", 22.0f); }
    static Action* bwl_ebonroc_taunt(PlayerbotAI* botAI) { return new BwlEbonrocTauntAction(botAI); }
    static Action* bwl_nefarian_drakonid_mark(PlayerbotAI* botAI)
    {
        using namespace BlackwingLairHelpers;
        return new RaidKillOrderMarkAction(botAI, "bwl nefarian drakonid mark", "", NEFARIAN_DRAKONIDS);
    }

    static Action* bwl_check_onyxia_scale_cloak(PlayerbotAI* ai) { return new BwlOnyxiaScaleCloakAuraCheckAction(ai); }
    static Action* bwl_turn_off_suppression_device(PlayerbotAI* ai) { return new BwlTurnOffSuppressionDeviceAction(ai); }
    static Action* bwl_razorgore_fire_resistance_action(PlayerbotAI* ai) { return new BossFireResistanceAction(ai, "razorgore the untamed"); }
    static Action* bwl_razorgore_avoid_aoe(PlayerbotAI* ai) { return new BwlRazorgoreAvoidAoeAction(ai); }
    static Action* bwl_razorgore_mark_boss(PlayerbotAI* ai) { return new BwlRazorgoreMarkBossAction(ai); }
    static Action* bwl_vaelastrasz_fire_resistance_action(PlayerbotAI* ai) { return new BossFireResistanceAction(ai, "vaelastrasz the corrupt"); }
    static Action* bwl_vaelastrasz_move_away(PlayerbotAI* ai) { return new BwlVaelastraszMoveAwayAction(ai); }
    static Action* bwl_use_hourglass_sand(PlayerbotAI* ai) { return new BwlUseHourglassSandAction(ai); }
    static Action* bwl_nefarian_fear_ward(PlayerbotAI* ai) { return new BwlNefarianFearWardAction(ai); }
    static Action* bwl_death_talon_wyrmguard_tank_move_away(PlayerbotAI* ai) { return new BwlDeathTalonWyrmguardTankMoveAwayAction(ai); }
    static Action* bwl_death_talon_wyrmguard_ranged_move_away(PlayerbotAI* ai) { return new BwlDeathTalonWyrmguardRangedMoveAwayAction(ai); }
};

#endif

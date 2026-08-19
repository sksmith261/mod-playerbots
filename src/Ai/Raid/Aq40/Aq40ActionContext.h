#ifndef PLAYERBOTS_AQ40ACTIONCONTEXT_H
#define PLAYERBOTS_AQ40ACTIONCONTEXT_H

#include "Action.h"
#include "NamedObjectContext.h"
#include "Aq40Actions.h"
#include "Aq40Utils.h"
#include "RaidBossScripts.h"

class RaidAq40ActionContext : public NamedObjectContext<Action>
{
public:
    RaidAq40ActionContext()
    {
        creators["aq40 exit stomach"] = &RaidAq40ActionContext::exit_stomach;
        creators["aq40 dodge dark glare"] = &RaidAq40ActionContext::dodge_dark_glare;
        creators["aq40 skeram mark"] = &RaidAq40ActionContext::skeram_mark;
        creators["aq40 sartura flee"] = &RaidAq40ActionContext::sartura_flee;
        creators["aq40 sartura mark"] = &RaidAq40ActionContext::sartura_mark;
        creators["aq40 bug trio mark"] = &RaidAq40ActionContext::bug_trio_mark;
        creators["aq40 fankriss worm mark"] = &RaidAq40ActionContext::fankriss_worm_mark;
        creators["aq40 twins retarget"] = &RaidAq40ActionContext::twins_retarget;
        creators["aq40 twins tank pickup"] = &RaidAq40ActionContext::twins_tank_pickup;
        creators["aq40 twins separate"] = &RaidAq40ActionContext::twins_separate;
        creators["aq40 twins caster range"] = &RaidAq40ActionContext::twins_caster_range;
        creators["aq40 twins team spacing"] = &RaidAq40ActionContext::twins_team_spacing;
        creators["aq40 twins tank drag"] = &RaidAq40ActionContext::twins_tank_drag;
        creators["aq40 twins marks"] = &RaidAq40ActionContext::twins_marks;
        creators["aq40 move from ground effect"] = &RaidAq40ActionContext::move_from_ground;
        creators["aq40 fear ward"] = &RaidAq40ActionContext::fear_ward;
        creators["aq40 flee mound"] = &RaidAq40ActionContext::flee_mound;
        creators["aq40 vem backup taunt"] = &RaidAq40ActionContext::vem_taunt;
        creators["aq40 skeram tank pickup"] = &RaidAq40ActionContext::skeram_pickup;
        creators["aq40 skeram healer follow"] = &RaidAq40ActionContext::skeram_healer_follow;
        creators["aq40 ouro backup taunt"] = &RaidAq40ActionContext::ouro_taunt;
        creators["aq40 huhuran standoff"] = &RaidAq40ActionContext::huhuran_standoff;
        creators["aq40 skeram standoff"] = &RaidAq40ActionContext::skeram_standoff;
        creators["aq40 giant claw sitter"] = &RaidAq40ActionContext::giant_claw_sitter;
        creators["aq40 cthun ring"] = &RaidAq40ActionContext::cthun_ring;
    }

private:
    static Action* exit_stomach(PlayerbotAI* ai) { return new Aq40ExitStomachAction(ai); }
    static Action* dodge_dark_glare(PlayerbotAI* ai) { return new Aq40DodgeDarkGlareAction(ai); }
    static Action* skeram_mark(PlayerbotAI* ai) { return new Aq40SkeramMarkAction(ai); }
    static Action* sartura_flee(PlayerbotAI* ai) { return new Aq40SarturaFleeAction(ai); }
    static Action* sartura_mark(PlayerbotAI* ai)
    { return new RaidKillOrderMarkAction(ai, "aq40 sartura mark", "battleguard sartura", { RaidAq40::NPC_SARTURA_ROYAL_GUARD }); }
    static Action* bug_trio_mark(PlayerbotAI* ai)
    {
        using namespace RaidAq40;
        // Yauj (heals) first, Kri second, Vem last: his death grants the
        // survivors Vengeance, so he dies once nothing benefits from it.
        return new RaidKillOrderMarkAction(ai, "aq40 bug trio mark", "", { NPC_PRINCESS_YAUJ, NPC_LORD_KRI, NPC_VEM });
    }
    static Action* fankriss_worm_mark(PlayerbotAI* ai)
    { return new RaidKillOrderMarkAction(ai, "aq40 fankriss worm mark", "fankriss the unyielding",
        { RaidAq40::NPC_VEKNISS_HATCHLING, RaidAq40::NPC_SPAWN_OF_FANKRISS }); }
    static Action* twins_retarget(PlayerbotAI* ai) { return new Aq40TwinsRetargetAction(ai); }
    static Action* twins_tank_pickup(PlayerbotAI* ai) { return new Aq40TwinsTankPickupAction(ai); }
    static Action* twins_separate(PlayerbotAI* ai) { return new Aq40TwinsSeparateAction(ai); }
    static Action* twins_marks(PlayerbotAI* ai) { return new Aq40TwinsMarkAction(ai); }
    static Action* twins_tank_drag(PlayerbotAI* ai) { return new Aq40TwinsTankDragAction(ai); }
    static Action* twins_team_spacing(PlayerbotAI* ai) { return new Aq40TwinsTeamSpacingAction(ai); }
    static Action* twins_caster_range(PlayerbotAI* ai) { return new Aq40TwinsCasterRangeAction(ai); }
    static Action* move_from_ground(PlayerbotAI* ai) { return new RaidMoveFromGroundEffectAction(ai, "aq40 move from ground effect"); }
    static Action* fear_ward(PlayerbotAI* ai) { return new RaidFearWardAction(ai, "aq40 fear ward"); }
    static Action* giant_claw_sitter(PlayerbotAI* ai) { return new Aq40GiantClawSitAction(ai); }
    static Action* cthun_ring(PlayerbotAI* ai) { return new Aq40CthunRingAction(ai); }
    static Action* huhuran_standoff(PlayerbotAI* ai)
    { return new RaidStandoffAction(ai, "aq40 huhuran standoff", "princess huhuran", 30.0f); }
    static Action* skeram_standoff(PlayerbotAI* ai)
    { return new RaidStandoffAction(ai, "aq40 skeram standoff", "the prophet skeram", 22.0f); }
    static Action* skeram_healer_follow(PlayerbotAI* ai) { return new Aq40SkeramHealerFollowAction(ai); }
    static Action* skeram_pickup(PlayerbotAI* ai) { return new Aq40SkeramTankPickupAction(ai); }
    static Action* vem_taunt(PlayerbotAI* ai)
    { return new RaidBackupTauntAction(ai, "aq40 vem backup taunt", "vem"); }
    static Action* ouro_taunt(PlayerbotAI* ai)
    { return new RaidBackupTauntAction(ai, "aq40 ouro backup taunt", "ouro"); }
    static Action* flee_mound(PlayerbotAI* ai)
    { return new MoveAwayFromCreatureAction(ai, "aq40 flee mound", RaidAq40::NPC_OURO_DIRT_MOUND, RaidAq40::OURO_MOUND_FLEE_RANGE); }
};

#endif

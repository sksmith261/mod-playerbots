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
    { return new RaidKillOrderMarkAction(ai, "aq40 fankriss worm mark", "fankriss the unyielding", { RaidAq40::NPC_SPAWN_OF_FANKRISS }); }
    static Action* twins_retarget(PlayerbotAI* ai) { return new Aq40TwinsRetargetAction(ai); }
};

#endif

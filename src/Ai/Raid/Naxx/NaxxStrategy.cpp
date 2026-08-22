#include "NaxxStrategy.h"

#include "NaxxMultipliers.h"

void RaidNaxxStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    // Keeps the raid plan fresh. Lowest priority in the list — it never
    // consumes a tick, it only ensures the plan exists for everything above
    // it to read.
    triggers.push_back(new TriggerNode("raid director", { NextAction("raid director tick", 1.0f) }));

    // Generic ground-hazard escape. Naxx had none wired at all, so Blaumeux's
    // Void Zones, Grobbulus' clouds and every other dynamic-object pool were
    // simply stood in.
    // Above the encounter actions, not level with them: Blaumeux drops her
    // Void Zone on her own tank, and at equal priority the camp anchor kept
    // dragging that tank back into it. Standing in a pool is never the
    // right answer, so escaping always wins.
    triggers.push_back(new TriggerNode("have area debuff", { NextAction("avoid aoe", ACTION_RAID + 2) }));

    // Grobbulus
    triggers.push_back(new TriggerNode("mutating injection melee",
        { NextAction("grobbulus move away", ACTION_RAID + 2) }
    ));

    triggers.push_back(new TriggerNode("mutating injection ranged",
        { NextAction("grobbulus go behind the boss", ACTION_RAID + 2) }
    ));

    triggers.push_back(new TriggerNode("mutating injection removed",
        { NextAction("grobbulus move center", ACTION_RAID + 1) }
    ));

    triggers.push_back(new TriggerNode("grobbulus cloud",
        { NextAction("rotate grobbulus", ACTION_RAID + 1) }
    ));

    // Heigan the Unclean — above avoid-aoe: the dance IS the aoe answer.
    triggers.push_back(new TriggerNode("heigan dance",
        { NextAction("heigan dance", ACTION_RAID + 3) }
    ));

    // Kel'Thuzad
    triggers.push_back(
        new TriggerNode("kel'thuzad",
        {
            NextAction("kel'thuzad position", ACTION_RAID + 2),
            NextAction("kel'thuzad choose target", ACTION_RAID + 1)
        })
    );

    // Anub'Rekhan
    triggers.push_back(new TriggerNode("anub'rekhan",
        { NextAction("anub'rekhan position", ACTION_RAID + 1),
          NextAction("anub'rekhan choose target", ACTION_RAID) }
    ));

    // Default posture between swarms; the swarm logic above outranks it.
    triggers.push_back(new TriggerNode("anub'rekhan spread",
        { NextAction("anub'rekhan spread", ACTION_RAID) }
    ));

     // Grand Widow Faerlina — preserve worshippers, hold them at the boss,
     // burn one on Frenzy (Widow's Embrace casts from its corpse).
     triggers.push_back(new TriggerNode("faerlina",
        { NextAction("avoid aoe", ACTION_RAID + 1) }
    ));

    triggers.push_back(new TriggerNode("faerlina worshipper duty",
        { NextAction("faerlina worshipper duty", ACTION_RAID + 1) }
    ));

    triggers.push_back(new TriggerNode("faerlina frenzy",
        { NextAction("faerlina sacrifice", ACTION_RAID + 2) }
    ));

    // Maexxna
    triggers.push_back(
        new TriggerNode("maexxna",
        {
            NextAction("rear flank", ACTION_RAID + 1),
            NextAction("avoid aoe", ACTION_RAID + 1)
        })
    );

    // Wrapped raiders die on the wall unless someone shoots them down.
    triggers.push_back(new TriggerNode("maexxna web wrap",
        { NextAction("maexxna free wrapped", ACTION_RAID + 2) }
    ));

    // Noth the Plaguebringer — skeleton discipline, both phases.
    triggers.push_back(new TriggerNode("noth",
        { NextAction("noth choose target", ACTION_RAID + 1) }
    ));

    // Gothik the Harvester — fight-time wave priorities; the pre-pull side
    // split is the raid leader's job (@group goto onto the dead side).
    triggers.push_back(new TriggerNode("gothik",
        { NextAction("gothik choose target", ACTION_RAID + 1) }
    ));

    // Patchwerk
    triggers.push_back(new TriggerNode("patchwerk tank",
        { NextAction("tank face", ACTION_RAID + 2) }
    ));

    triggers.push_back(new TriggerNode("patchwerk ranged",
        { NextAction("patchwerk ranged position", ACTION_RAID + 2) }
    ));

    triggers.push_back(new TriggerNode("patchwerk non-tank",
        { NextAction("rear flank", ACTION_RAID + 1) }
    ));

    // Thaddius — leash guard outranks all other movement: a dragged pet
    // snaps its coil tether and the coil shreds the raid.
    triggers.push_back(new TriggerNode("thaddius tether",
        { NextAction("thaddius tether", ACTION_RAID + 3) }
    ));

    // Thaddius
    triggers.push_back(new TriggerNode("thaddius phase pet",
        { NextAction("thaddius attack nearest pet", ACTION_RAID + 1) }
    ));

    triggers.push_back(new TriggerNode("thaddius phase pet lose aggro",
        { NextAction("taunt spell", ACTION_RAID + 2) }
    ));

    triggers.push_back(new TriggerNode("thaddius phase transition",
        { NextAction("thaddius move to platform", ACTION_RAID + 1) }
    ));

    triggers.push_back(new TriggerNode("thaddius phase thaddius",
        { NextAction("thaddius move polarity", ACTION_RAID + 1) }
    ));

    // Instructor Razuvious
    triggers.push_back(new TriggerNode("razuvious tank",
        { NextAction("razuvious use obedience crystal", ACTION_RAID + 1) }
    ));

    triggers.push_back(new TriggerNode("razuvious nontank",
        { NextAction("razuvious target", ACTION_RAID + 1) }
    ));

    // Four Horsemen: one duty action for the whole raid — stack-driven
    // pair rotation (front: Thane/Baron, back: Lady/Sir). Replaces the
    // attracter model (4 lone bots who never attacked) and its drifting
    // timer rotation.
    triggers.push_back(new TriggerNode("four horsemen duty",
        { NextAction("four horsemen duty", ACTION_RAID + 1) }
    ));

    // Sapphiron — the raid plan owns both phases: ground positioning and
    // damage, and the air phase where each bot is dealt an ice block to
    // break line of sight behind. The old ground/flight pair derived all of
    // that per bot, which meant a bot holding no threat on her (most of
    // them, most of the time) simply did nothing.
    triggers.push_back(new TriggerNode("sapphiron ground",
        { NextAction("sapphiron plan", ACTION_RAID + 2) }
    ));

    triggers.push_back(new TriggerNode("sapphiron flight",
        { NextAction("sapphiron plan", ACTION_RAID + 2) }
    ));

    // Gluth
    triggers.push_back(
        new TriggerNode("gluth",
        {
            NextAction("gluth choose target", ACTION_RAID + 1),
            NextAction("gluth position", ACTION_RAID + 1),
            NextAction("gluth slowdown", ACTION_RAID)
        })
    );

    triggers.push_back(new TriggerNode("gluth main tank mortal wound",
        { NextAction("taunt spell", ACTION_RAID + 1) }
    ));

    // Loatheb
    triggers.push_back(
        new TriggerNode("loatheb",
        {
            NextAction("loatheb position", ACTION_RAID + 1),
            NextAction("loatheb choose target", ACTION_RAID + 1)
        })
    );

    // Spore rotation: five unbuffed DPS at a time soak Fungal Creep.
    triggers.push_back(new TriggerNode("loatheb spore",
        { NextAction("loatheb spore soak", ACTION_RAID + 2) }
    ));

}

void RaidNaxxStrategy::InitMultipliers(std::vector<Multiplier*>& multipliers)
{
    multipliers.push_back(new GrobbulusMultiplier(botAI));
    multipliers.push_back(new FaerlinaDisciplineMultiplier(botAI));
    multipliers.push_back(new GothikBalconyMultiplier(botAI));
    //multipliers.push_back(new HeiganDanceMultiplier(botAI));
    multipliers.push_back(new LoathebGenericMultiplier(botAI));
    multipliers.push_back(new ThaddiusGenericMultiplier(botAI));
    multipliers.push_back(new SapphironGenericMultiplier(botAI));
    multipliers.push_back(new InstructorRazuviousGenericMultiplier(botAI));
    multipliers.push_back(new KelthuzadGenericMultiplier(botAI));
    multipliers.push_back(new AnubrekhanGenericMultiplier(botAI));
    multipliers.push_back(new FourHorsemenGenericMultiplier(botAI));
    // multipliers.push_back(new GothikGenericMultiplier(botAI));
    multipliers.push_back(new GluthGenericMultiplier(botAI));
}

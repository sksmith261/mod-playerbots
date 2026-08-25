/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_NAXXTRIGGERCONTEXT_H
#define PLAYERBOTS_NAXXTRIGGERCONTEXT_H

#include "NamedObjectContext.h"
#include "NaxxTriggers.h"
#include "RaidDirector.h"

class RaidNaxxTriggerContext : public NamedObjectContext<Trigger>
{
public:
    RaidNaxxTriggerContext()
    {
        creators["mutating injection melee"] = &RaidNaxxTriggerContext::mutating_injection_melee;
        creators["mutating injection ranged"] = &RaidNaxxTriggerContext::mutating_injection_ranged;
        creators["mutating injection removed"] = &RaidNaxxTriggerContext::mutating_injection_removed;
        creators["grobbulus cloud"] = &RaidNaxxTriggerContext::grobbulus_cloud;
        creators["grobbulus ranged position"] = &RaidNaxxTriggerContext::grobbulus_ranged_position;
        creators["heigan dance"] = &RaidNaxxTriggerContext::heigan_dance;
        creators["anub'rekhan spread"] = &RaidNaxxTriggerContext::anubrekhan_spread;
        creators["maexxna web wrap"] = &RaidNaxxTriggerContext::maexxna_web_wrap;
        creators["noth"] = &RaidNaxxTriggerContext::noth;
        creators["loatheb spore"] = &RaidNaxxTriggerContext::loatheb_spore;
        creators["gothik"] = &RaidNaxxTriggerContext::gothik;
        creators["four horsemen duty"] = &RaidNaxxTriggerContext::four_horsemen_duty;
        creators["thaddius tether"] = &RaidNaxxTriggerContext::thaddius_tether;
        creators["raid director"] = &RaidNaxxTriggerContext::raid_director;
        creators["naxx plan"] = &RaidNaxxTriggerContext::naxx_plan;
        creators["faerlina worshipper duty"] = &RaidNaxxTriggerContext::faerlina_worshipper_duty;
        creators["faerlina frenzy"] = &RaidNaxxTriggerContext::faerlina_frenzy;
        //creators["heigan melee"] = &RaidNaxxTriggerContext::heigan_melee;
        //creators["heigan ranged"] = &RaidNaxxTriggerContext::heigan_ranged;

        creators["thaddius phase pet"] = &RaidNaxxTriggerContext::thaddius_phase_pet;
        creators["thaddius phase pet lose aggro"] = &RaidNaxxTriggerContext::thaddius_phase_pet_lose_aggro;
        creators["thaddius phase transition"] = &RaidNaxxTriggerContext::thaddius_phase_transition;
        creators["thaddius phase thaddius"] = &RaidNaxxTriggerContext::thaddius_phase_thaddius;

        creators["razuvious tank"] = &RaidNaxxTriggerContext::razuvious_tank;
        creators["razuvious nontank"] = &RaidNaxxTriggerContext::razuvious_nontank;

        creators["four horsemen attractors"] = &RaidNaxxTriggerContext::four_horsemen_attractors;
        creators["four horsemen except attractors"] = &RaidNaxxTriggerContext::four_horsemen_except_attractors;

        creators["sapphiron ground"] = &RaidNaxxTriggerContext::sapphiron_ground;
        creators["sapphiron flight"] = &RaidNaxxTriggerContext::sapphiron_flight;

        creators["kel'thuzad"] = &RaidNaxxTriggerContext::kelthuzad;

        creators["anub'rekhan"] = &RaidNaxxTriggerContext::anubrekhan;
        creators["faerlina"] = &RaidNaxxTriggerContext::faerlina;
        creators["maexxna"] = &RaidNaxxTriggerContext::maexxna;
        creators["patchwerk tank"] = &RaidNaxxTriggerContext::patchwerk_tank;
        creators["patchwerk non-tank"] = &RaidNaxxTriggerContext::patchwerk_non_tank;
        creators["patchwerk ranged"] = &RaidNaxxTriggerContext::patchwerk_ranged;

        creators["gluth"] = &RaidNaxxTriggerContext::gluth;
        creators["gluth main tank mortal wound"] = &RaidNaxxTriggerContext::gluth_main_tank_mortal_wound;

        creators["loatheb"] = &RaidNaxxTriggerContext::loatheb;
    }

private:
    static Trigger* mutating_injection_melee(PlayerbotAI* ai) { return new MutatingInjectionMeleeTrigger(ai); }
    static Trigger* mutating_injection_ranged(PlayerbotAI* ai) { return new MutatingInjectionRangedTrigger(ai); }
    static Trigger* mutating_injection_removed(PlayerbotAI* ai) { return new MutatingInjectionRemovedTrigger(ai); }
    static Trigger* grobbulus_cloud(PlayerbotAI* ai) { return new GrobbulusCloudTrigger(ai); }
    static Trigger* grobbulus_ranged_position(PlayerbotAI* ai) { return new GrobbulusRangedPositionTrigger(ai); }
    static Trigger* heigan_dance(PlayerbotAI* ai) { return new HeiganDanceTrigger(ai); }
    static Trigger* anubrekhan_spread(PlayerbotAI* ai) { return new AnubrekhanSpreadTrigger(ai); }
    static Trigger* maexxna_web_wrap(PlayerbotAI* ai) { return new MaexxnaWebWrapTrigger(ai); }
    static Trigger* noth(PlayerbotAI* ai) { return new NothTrigger(ai); }
    static Trigger* loatheb_spore(PlayerbotAI* ai) { return new LoathebSporeTrigger(ai); }
    static Trigger* gothik(PlayerbotAI* ai) { return new GothikTrigger(ai); }
    static Trigger* four_horsemen_duty(PlayerbotAI* ai) { return new FourHorsemenDutyTrigger(ai); }
    static Trigger* thaddius_tether(PlayerbotAI* ai) { return new ThaddiusTetherTrigger(ai); }
    static Trigger* raid_director(PlayerbotAI* ai) { return new RaidDirectorTrigger(ai); }
    static Trigger* naxx_plan(PlayerbotAI* ai) { return new NaxxPlanTrigger(ai); }
    static Trigger* faerlina_worshipper_duty(PlayerbotAI* ai) { return new FaerlinaWorshipperDutyTrigger(ai); }
    static Trigger* faerlina_frenzy(PlayerbotAI* ai) { return new FaerlinaFrenzyTrigger(ai); }
    //static Trigger* heigan_melee(PlayerbotAI* ai) { return new HeiganMeleeTrigger(ai); }
    //static Trigger* heigan_ranged(PlayerbotAI* ai) { return new HeiganRangedTrigger(ai); }

    static Trigger* thaddius_phase_pet(PlayerbotAI* ai) { return new ThaddiusPhasePetTrigger(ai); }
    static Trigger* thaddius_phase_pet_lose_aggro(PlayerbotAI* ai) { return new ThaddiusPhasePetLoseAggroTrigger(ai); }
    static Trigger* thaddius_phase_transition(PlayerbotAI* ai) { return new ThaddiusPhaseTransitionTrigger(ai); }
    static Trigger* thaddius_phase_thaddius(PlayerbotAI* ai) { return new ThaddiusPhaseThaddiusTrigger(ai); }
    static Trigger* razuvious_tank(PlayerbotAI* ai) { return new RazuviousTankTrigger(ai); }
    static Trigger* razuvious_nontank(PlayerbotAI* ai) { return new RazuviousNontankTrigger(ai); }

    static Trigger* four_horsemen_attractors(PlayerbotAI* ai) { return new FourHorsemenAttractorsTrigger(ai); }
    static Trigger* four_horsemen_except_attractors(PlayerbotAI* ai) { return new FourHorsemenExceptAttractorsTrigger(ai); }

    static Trigger* sapphiron_ground(PlayerbotAI* ai) { return new SapphironGroundTrigger(ai); }
    static Trigger* sapphiron_flight(PlayerbotAI* ai) { return new SapphironFlightTrigger(ai); }
    static Trigger* kelthuzad(PlayerbotAI* ai) { return new KelthuzadTrigger(ai); }
    static Trigger* anubrekhan(PlayerbotAI* ai) { return new AnubrekhanTrigger(ai); }
    static Trigger* faerlina(PlayerbotAI* ai) { return new FaerlinaTrigger(ai); }
    static Trigger* maexxna(PlayerbotAI* ai) { return new MaexxnaTrigger(ai); }
    static Trigger* patchwerk_tank(PlayerbotAI* ai) { return new PatchwerkTankTrigger(ai); }
    static Trigger* patchwerk_non_tank(PlayerbotAI* ai) { return new PatchwerkNonTankTrigger(ai); }
    static Trigger* patchwerk_ranged(PlayerbotAI* ai) { return new PatchwerkRangedTrigger(ai); }
    static Trigger* gluth(PlayerbotAI* ai) { return new GluthTrigger(ai); }
    static Trigger* gluth_main_tank_mortal_wound(PlayerbotAI* ai) { return new GluthMainTankMortalWoundTrigger(ai); }
    static Trigger* loatheb(PlayerbotAI* ai) { return new LoathebTrigger(ai); }
};

#endif

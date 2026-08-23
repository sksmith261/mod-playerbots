#ifndef PLAYERBOTS_NAXXACTIONS_H
#define PLAYERBOTS_NAXXACTIONS_H

#include "Action.h"
#include "AttackAction.h"
#include "GenericActions.h"
#include "MovementActions.h"
#include "PlayerbotAI.h"
#include "Playerbots.h"
#include "NaxxBossHelper.h"

class GrobbulusGoBehindAction : public MovementAction
{
public:
    GrobbulusGoBehindAction(PlayerbotAI* ai, float distance = 24.0f, float delta_angle = M_PI / 8)
        : MovementAction(ai, "grobbulus go behind")
    {
        this->distance = distance;
        this->delta_angle = delta_angle;
    }
    virtual bool Execute(Event event);

protected:
    float distance, delta_angle;
};

class GrobbulusRotateAction : public RotateAroundTheCenterPointAction
{
public:
    GrobbulusRotateAction(PlayerbotAI* botAI)
        : RotateAroundTheCenterPointAction(botAI, "rotate grobbulus", 3281.23f, -3310.38f, 18.0f, 8, true, M_PI) {}
    virtual bool isUseful() override
    {
        return RotateAroundTheCenterPointAction::isUseful() && botAI->IsMainTank(bot) &&
               AI_VALUE2(bool, "has aggro", "boss target");
    }
    bool Execute(Event event) override;
    uint32 GetCurrWaypoint() override;
};

class GrobbulusMoveCenterAction : public MoveInsideAction
{
public:
    GrobbulusMoveCenterAction(PlayerbotAI* ai) : MoveInsideAction(ai, 3281.23f, -3310.38f, 5.0f) {}

    // Same bad anchor as the kite ring: re-target the boss's own spawn.

    // Base MoveInsideAction moves at MOVEMENT_NORMAL, which loses to
    // in-flight moves and the body-pull guard's veto. Walking back to the
    // raid after Mutating Injection is a mechanic move: COMBAT priority.
    bool Execute(Event event) override
    {
        float cx = x, cy = y;
        if (Unit* boss = AI_VALUE(Unit*, "boss target"))
            if (Creature* creature = boss->ToCreature())
            {
                cx = creature->GetHomePosition().GetPositionX();
                cy = creature->GetHomePosition().GetPositionY();
            }

        return MoveInside(bot->GetMapId(), cx, cy, bot->GetPositionZ(), distance,
                          MovementPriority::MOVEMENT_COMBAT);
    }
};

class GrobbulusMoveAwayAction : public MovementAction
{
public:
    GrobbulusMoveAwayAction(PlayerbotAI* ai, float distance = 18.0f)
        : MovementAction(ai, "grobbulus move away"), distance(distance)
    {
    }
    bool Execute(Event event) override;

private:
    float distance;
};

// Stack-driven pair rotation for the whole raid. Front team (tanks and
// melee) rotates between Thane and Baron's corner stations; back team
// (ranged and healers) between Lady and Sir's. A bot swaps sides when its
// own stack of its current side's Mark reaches 3 — the debuff is the
// clock, so nothing drifts. Side parity comes from a shared-order
// election; the flip state lives in the action object.
class FourHorsemenDutyAction : public AttackAction
{
public:
    FourHorsemenDutyAction(PlayerbotAI* ai) : AttackAction(ai, "four horsemen duty"), helper(ai) {}
    bool Execute(Event event) override;

private:
    FourHorsemenBossHelper helper;
};

class MaexxnaFreeWrappedAction : public AttackAction
{
public:
    MaexxnaFreeWrappedAction(PlayerbotAI* ai) : AttackAction(ai, "maexxna free wrapped") {}
    bool Execute(Event event) override;
};

class NothChooseTargetAction : public AttackAction
{
public:
    NothChooseTargetAction(PlayerbotAI* ai) : AttackAction(ai, "noth choose target") {}
    bool Execute(Event event) override;
};

class LoathebSporeSoakAction : public AttackAction
{
public:
    LoathebSporeSoakAction(PlayerbotAI* ai) : AttackAction(ai, "loatheb spore soak") {}
    bool Execute(Event event) override;
};

class GothikChooseTargetAction : public AttackAction
{
public:
    GothikChooseTargetAction(PlayerbotAI* ai) : AttackAction(ai, "gothik choose target") {}
    bool Execute(Event event) override;
};

class AnubrekhanSpreadAction : public MovementAction
{
public:
    AnubrekhanSpreadAction(PlayerbotAI* ai) : MovementAction(ai, "anub'rekhan spread") {}
    bool Execute(Event event) override;
};

// Assist tanks each hold one worshipper (GUID-sorted roster, slot by
// assist-tank index) and drag it into Widow's Embrace range of the boss.
class FaerlinaWorshipperDutyAction : public AttackAction
{
public:
    FaerlinaWorshipperDutyAction(PlayerbotAI* ai) : AttackAction(ai, "faerlina worshipper duty") {}
    bool Execute(Event event) override;
};

// Frenzy is up: everyone burns the elected sacrifice worshipper.
class FaerlinaSacrificeAction : public AttackAction
{
public:
    FaerlinaSacrificeAction(PlayerbotAI* ai) : AttackAction(ai, "faerlina sacrifice") {}
    bool Execute(Event event) override;
};

// Server-side safety dance. The reizan-core instance script mirrors
// boss_heigan's section rotation and exposes the NEXT safe section via
// GetData(DATA_HEIGAN_ERUPTION); this action just stands in it. The old
// (commented) implementation guessed sections by watching Heigan's cast
// state, which never fires — eruptions are instance-driven GO casts.
class HeiganDanceAction : public MovementAction
{
public:
    HeiganDanceAction(PlayerbotAI* ai) : MovementAction(ai, "heigan dance") {}
    bool Execute(Event event) override;
};

//class HeiganDanceAction : public MovementAction
//{
//public:
//    HeiganDanceAction(PlayerbotAI* ai) : MovementAction(ai, "heigan dance")
//    {
//        this->last_eruption_ms = 0;
//        this->platform_phase = false;
//        ResetSafe();
//        waypoints.push_back(std::make_pair(2794.88f, -3668.12f));
//        waypoints.push_back(std::make_pair(2775.49f, -3674.43f));
//        waypoints.push_back(std::make_pair(2762.30f, -3684.59f));
//        waypoints.push_back(std::make_pair(2755.99f, -3703.96f));
//        platform = std::make_pair(2794.26f, -3706.67f);
//    }
//
//protected:
//    bool CalculateSafe();
//    void ResetSafe()
//    {
//        curr_safe = 0;
//        curr_dir = 1;
//    }
//    void NextSafe()
//    {
//        curr_safe += curr_dir;
//        if (curr_safe == 3 || curr_safe == 0)
//        {
//            curr_dir = -curr_dir;
//        }
//    }
//    uint32 last_eruption_ms;
//    bool platform_phase;
//    uint32 curr_safe, curr_dir;
//    std::vector<std::pair<float, float>> waypoints;
//    std::pair<float, float> platform;
//};
//
//class HeiganDanceMeleeAction : public HeiganDanceAction
//{
//public:
//    HeiganDanceMeleeAction(PlayerbotAI* ai) : HeiganDanceAction(ai) {}
//    virtual bool Execute(Event event);
//};
//
//class HeiganDanceRangedAction : public HeiganDanceAction
//{
//public:
//    HeiganDanceRangedAction(PlayerbotAI* ai) : HeiganDanceAction(ai) {}
//    virtual bool Execute(Event event);
//};

class ThaddiusTetherAction : public MovementAction
{
public:
    ThaddiusTetherAction(PlayerbotAI* ai) : MovementAction(ai, "thaddius tether"), helper(ai) {}
    bool Execute(Event event) override;

private:
    ThaddiusBossHelper helper;
};

class ThaddiusAttackNearestPetAction : public AttackAction
{
public:
    ThaddiusAttackNearestPetAction(PlayerbotAI* ai) : AttackAction(ai, "thaddius attack nearest pet"), helper(ai) {}
    virtual bool Execute(Event event);
    virtual bool isUseful();

private:
    ThaddiusBossHelper helper;
};

// class ThaddiusMeleeToPlaceAction : public MovementAction
// {
// public:
//     ThaddiusMeleeToPlaceAction(PlayerbotAI* ai) : MovementAction(ai, "thaddius melee to place") {}
//     virtual bool Execute(Event event);
//     virtual bool isUseful();
// };

// class ThaddiusRangedToPlaceAction : public MovementAction
// {
// public:
//     ThaddiusRangedToPlaceAction(PlayerbotAI* ai) : MovementAction(ai, "thaddius ranged to place") {}
//     virtual bool Execute(Event event);
//     virtual bool isUseful();
// };

class ThaddiusMoveToPlatformAction : public MovementAction
{
public:
    ThaddiusMoveToPlatformAction(PlayerbotAI* ai) : MovementAction(ai, "thaddius move to platform") {}
    virtual bool Execute(Event event);
    virtual bool isUseful();
};

class ThaddiusMovePolarityAction : public MovementAction
{
public:
    ThaddiusMovePolarityAction(PlayerbotAI* ai) : MovementAction(ai, "thaddius move polarity") {}
    virtual bool Execute(Event event);
    virtual bool isUseful();
};

class RazuviousUseObedienceCrystalAction : public MovementAction
{
public:
    RazuviousUseObedienceCrystalAction(PlayerbotAI* ai)
        : MovementAction(ai, "razuvious use obedience crystal"), helper(ai)
    {
    }
    bool Execute(Event event) override;

private:
    RazuviousBossHelper helper;
};

class RazuviousTargetAction : public AttackAction
{
public:
    RazuviousTargetAction(PlayerbotAI* ai) : AttackAction(ai, "razuvious target"), helper(ai) {}
    bool Execute(Event event) override;

private:
    RazuviousBossHelper helper;
};

class FourHorsemenAttractAlternativelyAction : public AttackAction
{
public:
    FourHorsemenAttractAlternativelyAction(PlayerbotAI* ai) : AttackAction(ai, "four horsemen attract alternatively"), helper(ai)
    {
    }
    bool Execute(Event event) override;

protected:
    FourHorsemenBossHelper helper;
};

class FourHorsemenAttackInOrderAction : public AttackAction
{
public:
    FourHorsemenAttackInOrderAction(PlayerbotAI* ai) : AttackAction(ai, "four horsemen attack in order"), helper(ai) {}
    bool Execute(Event event) override;

protected:
    FourHorsemenBossHelper helper;
};

// class SapphironGroundMainTankPositionAction : public MovementAction
// {
// public:
//     SapphironGroundMainTankPositionAction(PlayerbotAI* ai) : MovementAction(ai, "sapphiron ground main tank
//     position") {} virtual bool Execute(Event event);
// };

// Carries out the raid plan's Sapphiron assignment (ground positioning and
// damage, or taking cover behind an assigned ice block in the air phase).
// Baseline execution for Naxx bosses with no bespoke handling.
class NaxxPlanAction : public AttackAction
{
public:
    NaxxPlanAction(PlayerbotAI* ai) : AttackAction(ai, "naxx plan") {}
    bool Execute(Event event) override;
};

class SapphironPlanAction : public AttackAction
{
public:
    SapphironPlanAction(PlayerbotAI* ai) : AttackAction(ai, "sapphiron plan") {}
    bool Execute(Event event) override;
};

class SapphironGroundPositionAction : public MovementAction
{
public:
    SapphironGroundPositionAction(PlayerbotAI* ai) : MovementAction(ai, "sapphiron ground position"), helper(ai) {}
    bool Execute(Event event) override;

protected:
    SapphironBossHelper helper;
};

class SapphironFlightPositionAction : public MovementAction
{
public:
    SapphironFlightPositionAction(PlayerbotAI* ai) : MovementAction(ai, "sapphiron flight position"), helper(ai) {}
    bool Execute(Event event) override;

protected:
    SapphironBossHelper helper;
    bool MoveToNearestIcebolt();
};

// class SapphironAvoidChillAction : public MovementAction
// {
// public:
//     SapphironAvoidChillAction(PlayerbotAI* ai) : MovementAction(ai, "sapphiron avoid chill") {}
//     virtual bool Execute(Event event);
// };

class KelthuzadChooseTargetAction : public AttackAction
{
public:
    KelthuzadChooseTargetAction(PlayerbotAI* ai) : AttackAction(ai, "kel'thuzad choose target"), helper(ai) {}
    virtual bool Execute(Event event);

private:
    KelthuzadBossHelper helper;
};

class KelthuzadPositionAction : public MovementAction
{
public:
    KelthuzadPositionAction(PlayerbotAI* ai) : MovementAction(ai, "kel'thuzad position"), helper(ai) {}
    virtual bool Execute(Event event);

private:
    KelthuzadBossHelper helper;
};

class AnubrekhanChooseTargetAction : public AttackAction
{
public:
    AnubrekhanChooseTargetAction(PlayerbotAI* ai) : AttackAction(ai, "anub'rekhan choose target") {}
    bool Execute(Event event) override;
};

class AnubrekhanPositionAction : public RotateAroundTheCenterPointAction
{
public:
    AnubrekhanPositionAction(PlayerbotAI* ai)
        : RotateAroundTheCenterPointAction(ai, "anub'rekhan position", 3272.49f, -3476.27f, 45.0f, 16) {}
    bool Execute(Event event) override;
};

class GluthChooseTargetAction : public AttackAction
{
public:
    GluthChooseTargetAction(PlayerbotAI* ai) : AttackAction(ai, "gluth choose target"), helper(ai) {}
    bool Execute(Event event) override;

private:
    GluthBossHelper helper;
};

class GluthPositionAction : public RotateAroundTheCenterPointAction
{
public:
    GluthPositionAction(PlayerbotAI* ai)
        : RotateAroundTheCenterPointAction(ai, "gluth position", 3293.61f, -3149.01f, 12.0f, 12), helper(ai) {}
    bool Execute(Event event) override;

private:
    GluthBossHelper helper;
};

class GluthSlowdownAction : public Action
{
public:
    GluthSlowdownAction(PlayerbotAI* ai) : Action(ai, "gluth slowdown"), helper(ai) {}
    bool Execute(Event event) override;

private:
    GluthBossHelper helper;
};

class LoathebPositionAction : public MovementAction
{
public:
    LoathebPositionAction(PlayerbotAI* ai) : MovementAction(ai, "loatheb position"), helper(ai) {}
    virtual bool Execute(Event event);

private:
    LoathebBossHelper helper;
};

class LoathebChooseTargetAction : public AttackAction
{
public:
    LoathebChooseTargetAction(PlayerbotAI* ai) : AttackAction(ai, "loatheb choose target"), helper(ai) {}
    virtual bool Execute(Event event);

private:
    LoathebBossHelper helper;
};

class PatchwerkRangedPositionAction : public MovementAction
{
public:
    PatchwerkRangedPositionAction(PlayerbotAI* ai) : MovementAction(ai, "patchwerk ranged position") {}
    bool Execute(Event event) override;
};

#endif

#ifndef PLAYERBOTS_MCACTIONS_H
#define PLAYERBOTS_MCACTIONS_H

#include <vector>

#include "AttackAction.h"
#include "MovementActions.h"
#include "PlayerbotAI.h"
#include "Playerbots.h"

class McMoveFromGroupAction : public MovementAction
{
public:
    McMoveFromGroupAction(PlayerbotAI* botAI, std::string const name = "mc move from group")
        : MovementAction(botAI, name) {}
    bool Execute(Event event) override;
};

class McMoveFromBaronGeddonAction : public MovementAction
{
public:
    McMoveFromBaronGeddonAction(PlayerbotAI* botAI, std::string const name = "mc move from baron geddon")
        : MovementAction(botAI, name) {}
    bool Execute(Event event) override;
};

class McShazzrahMoveAwayAction : public MovementAction
{
public:
    McShazzrahMoveAwayAction(PlayerbotAI* botAI, std::string const name = "mc shazzrah move away")
        : MovementAction(botAI, name) {}
    bool Execute(Event event) override;
};

// Shamans Purge / priests offensively Dispel Magic Shazzrah's Deaden Magic
// (-50% magic taken) so casters do full damage.
class McShazzrahPurgeAction : public Action
{
public:
    McShazzrahPurgeAction(PlayerbotAI* botAI, std::string const name = "mc shazzrah purge")
        : Action(botAI, name) {};
    bool Execute(Event event) override;
};

class McGolemaggMarkBossAction : public Action
{
public:
    McGolemaggMarkBossAction(PlayerbotAI* botAI, std::string const name = "mc golemagg mark boss")
        : Action(botAI, name) {};
    bool Execute(Event event) override;
};

class McGolemaggTankAction : public AttackAction
{
public:
    McGolemaggTankAction(PlayerbotAI* botAI, std::string const name)
        : AttackAction(botAI, name) {}
protected:
    bool MoveUnitToPosition(Unit* target, const Position& tankPosition, float maxDistance, float stepDistance = 3.0f);
    bool FindCoreRagers(Unit*& coreRager1, Unit*& coreRager2) const;
};

class McGolemaggMainTankAttackGolemaggAction : public McGolemaggTankAction
{
public:
    McGolemaggMainTankAttackGolemaggAction(PlayerbotAI* botAI, std::string const name = "mc golemagg main tank attack golemagg")
        : McGolemaggTankAction(botAI, name) {};
    bool Execute(Event event) override;
};

class McGolemaggAssistTankAttackCoreRagerAction : public McGolemaggTankAction
{
public:
    McGolemaggAssistTankAttackCoreRagerAction(PlayerbotAI* botAI, std::string const name = "mc golemagg assist tank attack core rager")
        : McGolemaggTankAction(botAI, name) {};
    bool Execute(Event event) override;
};

class McCoreHoundMarkAction : public Action
{
public:
    McCoreHoundMarkAction(PlayerbotAI* botAI, std::string const name = "mc core hound mark")
        : Action(botAI, name) {};
    Unit* GetTarget() override;
    bool Execute(Event event) override;
};

namespace MoltenCoreHelpers
{
// Living Firesworn sorted by GUID: a deterministic order every bot computes
// identically, used for banish assignments and kill-order marking.
std::vector<Unit*> GetLivingFiresworn(PlayerbotAI* botAI);
// The first min(2, group warlock count) of the sorted list are banish
// assignments; returns this bot's assigned target (warlock ranks 0/1 only),
// or nullptr.
Unit* GetGarrBanishAssignment(PlayerbotAI* botAI, Player* bot);
uint32 CountGarrBanishAssignments(PlayerbotAI* botAI, Player* bot);
bool IsBanished(Unit* unit);
}

// Warlocks 0/1 keep their assigned Firesworn banished for the whole fight.
class McGarrBanishAction : public Action
{
public:
    McGarrBanishAction(PlayerbotAI* botAI, std::string const name = "mc garr banish")
        : Action(botAI, name) {};
    bool Execute(Event event) override;
};

// Skull the current kill target: most-damaged living Firesworn that is
// neither banished nor banish-assigned; Garr once only those remain.
class McGarrMarkAction : public Action
{
public:
    McGarrMarkAction(PlayerbotAI* botAI, std::string const name = "mc garr mark")
        : Action(botAI, name) {};
    Unit* GetTarget() override;
    bool Execute(Event event) override;
};

// Generic fear-ward/ground-effect/kill-order actions now live in
// src/Ai/Raid/RaidBossScripts.h; only MC-specific actions remain here.

#endif

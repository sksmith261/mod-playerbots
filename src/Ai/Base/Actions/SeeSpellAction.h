/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_SEESPELLACTION_H
#define PLAYERBOTS_SEESPELLACTION_H

#include "MovementActions.h"

class Creature;
class Player;
class PlayerbotAI;
class WorldPosition;

class SeeSpellAction : public MovementAction
{
public:
    SeeSpellAction(PlayerbotAI* botAI, std::string const name = "see spell") : MovementAction(botAI, name) {}

    bool Execute(Event event) override;

    bool SelectSpell(WorldPosition& spellPosition);
    bool MoveSpell(WorldPosition& spellPosition);

    virtual bool MoveToSpell(WorldPosition& spellPosition, bool inFormation = true);
    void SetFormationOffset(WorldPosition& spellPosition);

    // spread/stack/goto: place this bot on its slot of a ring centered on the
    // clicked point. armed is the full armed command, e.g. "spread 5.00";
    // once used it is tagged "spread 5.00|<x>,<y>" (spent on that click).
    bool MoveToClickFormation(WorldPosition& center, std::string const& armed);
    static std::string ClickToken(std::string const& armed);
    static bool IsClickCommand(std::string const& armed);

private:
    Creature* CreateWps(Player* wpOwner, float x, float y, float z, float o, uint32 entry, Creature* lastWp,
                        bool important = false);
};

#endif

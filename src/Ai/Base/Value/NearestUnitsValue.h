/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_NEARESTUNITSVALUE_H
#define PLAYERBOTS_NEARESTUNITSVALUE_H

#include "PlayerbotAIConfig.h"
#include "Unit.h"
#include "Value.h"

class PlayerbotAI;

// Calculate() runs a Cell::VisitObjects grid sweep and then, unless ignoreLos is set, one
// vmap line-of-sight raycast per surviving candidate. That raycast is the most expensive
// thing a bot value does: BIH::intersectRay / intersectPoint / IntersectTriangle together
// measured ~8.5% of total worldserver cycles under 1200 bots.
//
// The default checkInterval used to be 1, which CalculatedValue::Get treats as "no cache,
// recalculate on every access". "possible targets" alone is read from ~50 call sites, so a
// single bot tick could re-run the same sweep and the same raycasts many times over.
//
// 100ms is deliberately chosen to be behaviourally free rather than a tuning compromise:
// the AI's own react rate (AiPlayerbot.ReactDelay, 100 by default) means a bot cannot act
// on a fresher result than this anyway, so collapsing repeat reads within one tick removes
// duplicated work without changing what any bot is able to do.
//
// Do not lower this below 100. CalculatedValue's constructor interprets an interval under
// 100 as *seconds* (checkInterval < 100 ? checkInterval * 1000 : checkInterval), so a value
// like 50 would silently become a 50-second cache.
static constexpr uint32 NEAREST_UNITS_DEFAULT_CACHE_MS = 100;

class NearestUnitsValue : public ObjectGuidListCalculatedValue
{
public:
    NearestUnitsValue(PlayerbotAI* botAI, std::string const name = "nearest units",
                      float range = sPlayerbotAIConfig.sightDistance, bool ignoreLos = false,
                      uint32 checkInterval = NEAREST_UNITS_DEFAULT_CACHE_MS)
        : ObjectGuidListCalculatedValue(botAI, name, checkInterval), range(range), ignoreLos(ignoreLos)
    {
    }

    GuidVector Calculate() override;

protected:
    virtual void FindUnits(std::list<Unit*>& targets) = 0;
    virtual bool AcceptUnit(Unit* unit) = 0;

    float range;
    bool ignoreLos;
};

#endif

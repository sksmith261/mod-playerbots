#ifndef PLAYERBOTS_NAXXBOSSHELPER_H
#define PLAYERBOTS_NAXXBOSSHELPER_H

#include <algorithm>
#include <string>

#include "AiObject.h"
#include "AiObjectContext.h"
#include "EventMap.h"
#include "Log.h"
#include "NamedObjectContext.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "PlayerbotAI.h"
#include "Playerbots.h"
#include "ScriptedCreature.h"
#include "SharedDefines.h"
#include "Spell.h"
#include "Timer.h"
#include "NaxxSpellIds.h"

const uint32 NAXX_MAP_ID = 533;

#include <cmath>

namespace NaxxHelpers
{
// Anub'Rekhan: single source of truth for "the swarm is happening" —
// shared by the kite/center logic and the default spread so they can
// never disagree about whose turn it is to own movement.
inline bool AnubrekhanSwarmActive(PlayerbotAI* botAI, Unit* boss)
{
    if (botAI->HasAura("locust swarm", boss))
        return true;

    if (Spell* spell = boss->GetCurrentSpell(CURRENT_GENERIC_SPELL))
        if (NaxxSpellIds::MatchesAnySpellId(spell->GetSpellInfo(),
                {NaxxSpellIds::LocustSwarm10, NaxxSpellIds::LocustSwarm10Alt, NaxxSpellIds::LocustSwarm25}))
            return true;

    return false;
}

// Assigned half-circle on the west side of Anub'Rekhan's room (his spawn
// is east of the ring center) for ranged and healers outside the swarm.
inline void AnubrekhanSpreadSlot(PlayerbotAI* botAI, Player* bot, float& x, float& y)
{
    float const centerX = 3272.49f, centerY = -3476.27f;
    int32 slot = botAI->GetGroupSlotIndex(bot);
    if (slot < 0)
        slot = 0;

    float const radius = PlayerbotAI::IsHeal(bot) ? 20.0f : 26.0f;
    float const angle = static_cast<float>(M_PI) + (slot - 20) * 0.025f * static_cast<float>(M_PI);
    x = centerX + radius * std::cos(angle);
    y = centerY + radius * std::sin(angle);
}

// Faerlina: live worshippers, GUID-sorted so every bot sees one roster.
inline std::vector<Unit*> FaerlinaWorshippers(PlayerbotAI* botAI)
{
    std::vector<Unit*> list;
    for (auto const& guid : botAI->GetAiObjectContext()->GetValue<GuidVector>("attackers")->Get())
    {
        Unit* unit = botAI->GetUnit(guid);
        if (unit && unit->IsAlive() && botAI->EqualLowercaseName(unit->GetName(), "naxxramas worshipper"))
            list.push_back(unit);
    }

    std::sort(list.begin(), list.end(), [](Unit* a, Unit* b) { return a->GetGUID() < b->GetGUID(); });
    return list;
}

// The worshipper to burn during Frenzy: lowest health among those close
// enough to Faerlina for Widow's Embrace to land on death (SmartAI casts
// it from the corpse; give it generous range margin).
inline Unit* FaerlinaSacrificeTarget(PlayerbotAI* botAI, Unit* faerlina)
{
    Unit* best = nullptr;
    for (Unit* worshipper : FaerlinaWorshippers(botAI))
    {
        if (worshipper->GetDistance(faerlina) > 30.0f)
            continue;

        if (!best || worshipper->GetHealthPct() < best->GetHealthPct())
            best = worshipper;
    }

    return best;
}
constexpr uint32 NPC_WEB_WRAP = 16486;

// Web Wraps by NAME: the wotlk entry is 16486 but IP's naxx-40 clones use
// offset entries — name-keyed search covers both versions.
inline std::vector<Unit*> MaexxnaWebWraps(PlayerbotAI* botAI, Player* bot)
{
    std::vector<Unit*> wraps;
    for (auto const& guid : botAI->GetAiObjectContext()->GetValue<GuidVector>("nearest npcs")->Get())
    {
        Unit* unit = botAI->GetUnit(guid);
        if (unit && unit->IsAlive() && botAI->EqualLowercaseName(unit->GetName(), "web wrap"))
            wraps.push_back(unit);
    }

    std::sort(wraps.begin(), wraps.end(), [](Unit* a, Unit* b) { return a->GetGUID() < b->GetGUID(); });
    return wraps;
}

// Spore 16286's on-death cast (smart_scripts): the soak buff. Matched by
// id — the name differs by era write-up (Fungal Creep / Fungal Bloom).
constexpr uint32 SPELL_SPORE_BUFF = 29232;

inline bool HasSporeBuff(PlayerbotAI* botAI, Player* player)
{
    return player->HasAura(SPELL_SPORE_BUFF) || botAI->HasAura("fungal creep", player) ||
           botAI->HasAura("fungal bloom", player);
}

// Noth's skeleton adds, both phases.
inline bool IsNothAdd(PlayerbotAI* botAI, Unit* unit)
{
    return botAI->EqualLowercaseName(unit->GetName(), "plagued warrior") ||
           botAI->EqualLowercaseName(unit->GetName(), "plagued champion") ||
           botAI->EqualLowercaseName(unit->GetName(), "plagued guardian");
}

// Gothik wave adds; higher rank dies first. 0 = not a Gothik add.
inline int32 GothikAddRank(PlayerbotAI* botAI, Unit* unit)
{
    std::string name = unit->GetName();
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    if (name.find("rider") != std::string::npos && name.find("unrelenting") == 0)
        return 3;
    if (name == "spectral rider")
        return 3;
    if (name.find("death knight") != std::string::npos &&
        (name.find("unrelenting") == 0 || name.find("spectral") == 0))
        return 2;
    if (name == "unrelenting trainee" || name == "spectral trainee" || name == "spectral horse")
        return 1;
    return 0;
}

// Loatheb: nearest live Spore (grid, legal — callers gate on Loatheb threat).
inline Unit* NearestLoathebSpore(PlayerbotAI* botAI, Player* bot)
{
    Unit* best = nullptr;
    for (auto const& guid : botAI->GetAiObjectContext()->GetValue<GuidVector>("nearest npcs")->Get())
    {
        Unit* unit = botAI->GetUnit(guid);
        if (!unit || !unit->IsAlive() || !botAI->EqualLowercaseName(unit->GetName(), "spore"))
            continue;

        if (!best || bot->GetDistance(unit) < bot->GetDistance(best))
            best = unit;
    }

    return best;
}

// The soak roster: the first five living DPS bots still missing Fungal
// Creep, in shared group order — every bot computes the same five, and the
// roster advances itself as buffs are gained (90s buff vs 13s spawns).
inline bool IsSporeSoaker(PlayerbotAI* botAI, Player* bot)
{
    Group* group = bot->GetGroup();
    if (!group)
        return false;

    uint32 count = 0;
    for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
    {
        Player* member = itr->GetSource();
        if (!member || !member->IsAlive() || !GET_PLAYERBOT_AI(member))
            continue;

        if (PlayerbotAI::IsTank(member) || PlayerbotAI::IsHeal(member))
            continue;

        if (HasSporeBuff(botAI, member))
            continue;

        if (member == bot)
            return true;

        if (++count >= 5)
            return false;
    }

    return false;
}
// Thaddius polarity: 0 = negative, 1 = positive, -1 = uncharged (the strip
// gap inside every shift). Works on any member, so bots can compute each
// other's cluster membership.
inline int32 ThaddiusCharge(PlayerbotAI* botAI, Player* player)
{
    if (NaxxSpellIds::HasAnyAura(player, {NaxxSpellIds::NegativeCharge10, NaxxSpellIds::NegativeCharge25,
                                          NaxxSpellIds::NegativeChargeStack}) ||
        botAI->HasAura("negative charge", player, false, false, -1, true))
        return 0;

    if (NaxxSpellIds::HasAnyAura(player, {NaxxSpellIds::PositiveCharge10, NaxxSpellIds::PositiveCharge25,
                                          NaxxSpellIds::PositiveChargeStack}) ||
        botAI->HasAura("positive charge", player, false, false, -1, true))
        return 1;

    return -1;
}
}  // namespace NaxxHelpers

template <class BossAiType>
class GenericBossHelper : public AiObject
{
public:
    GenericBossHelper(PlayerbotAI* botAI, std::string name) : AiObject(botAI), _name(name) {}
    virtual bool UpdateBossAI()
    {
        if (!bot->IsInCombat())
            _unit = nullptr;

        if (_unit && (!_unit->IsInWorld() || !_unit->IsAlive()))
            _unit = nullptr;

        if (!_unit)
        {
            _unit = AI_VALUE2(Unit*, "find target", _name);
            if (!_unit)
                return false;

            _target = _unit->ToCreature();
            if (!_target)
                return false;

            _ai = dynamic_cast<BossAiType*>(_target->GetAI());
            if (!_ai)
                return false;

            _event_map = &_ai->events;
            if (!_event_map)
                return false;
        }
        if (!_event_map)
            return false;

        _timer = getMSTime();
        return true;
    }
    virtual void Reset()
    {
        _unit = nullptr;
        _target = nullptr;
        _ai = nullptr;
        _event_map = nullptr;
        _timer = 0;
    }

protected:
    std::string _name;
    Unit* _unit = nullptr;
    Creature* _target = nullptr;
    BossAiType* _ai = nullptr;
    EventMap* _event_map = nullptr;
    uint32 _timer = 0;
};

class KelthuzadBossHelper : public AiObject
{
public:
    KelthuzadBossHelper(PlayerbotAI* botAI) : AiObject(botAI) {}
    const std::pair<float, float> center = {3716.19f, -5106.58f};
    const std::pair<float, float> tank_pos = {3709.19f, -5104.86f};
    const std::pair<float, float> assist_tank_pos = {3746.05f, -5112.74f};
    bool UpdateBossAI()
    {
        if (!bot->IsInCombat())
            Reset();

        if (_unit && (!_unit->IsInWorld() || !_unit->IsAlive()))
            Reset();

        if (!_unit)
            _unit = AI_VALUE2(Unit*, "find target", "kel'thuzad");

        return _unit != nullptr;
    }
    bool IsPhaseOne() { return _unit && _unit->HasUnitFlag(UNIT_FLAG_NON_ATTACKABLE); }
    bool IsPhaseTwo() { return _unit && !_unit->HasUnitFlag(UNIT_FLAG_NON_ATTACKABLE); }
    Unit* GetAnyShadowFissure()
    {
        Unit* shadow_fissure = nullptr;
        GuidVector units = *context->GetValue<GuidVector>("nearest triggers");
        for (auto i = units.begin(); i != units.end(); i++)
        {
            Unit* unit = botAI->GetUnit(*i);
            if (!unit)
                continue;
            if (botAI->EqualLowercaseName(unit->GetName(), "shadow fissure"))
                shadow_fissure = unit;
        }
        return shadow_fissure;
    }

private:
    void Reset() { _unit = nullptr; }

    Unit* _unit = nullptr;
};

class RazuviousBossHelper : public AiObject
{
public:
    RazuviousBossHelper(PlayerbotAI* botAI) : AiObject(botAI) {}
    bool UpdateBossAI()
    {
        if (!bot->IsInCombat())
            Reset();

        if (_unit && (!_unit->IsInWorld() || !_unit->IsAlive()))
            Reset();

        if (!_unit)
            _unit = AI_VALUE2(Unit*, "find target", "instructor razuvious");

        return _unit != nullptr;
    }

private:
    void Reset() { _unit = nullptr; }

    Unit* _unit = nullptr;
};

class SapphironBossHelper : public AiObject
{
public:
    const std::pair<float, float> mainTankPos = {3512.07f, -5274.06f};
    const std::pair<float, float> center = {3517.31f, -5253.74f};
    const float GENERIC_HEIGHT = 137.29f;
    SapphironBossHelper(PlayerbotAI* botAI) : AiObject(botAI) {}
    bool UpdateBossAI()
    {
        if (!bot->IsInCombat())
            Reset();

        if (_unit && (!_unit->IsInWorld() || !_unit->IsAlive()))
            Reset();

        if (!_unit)
        {
            _unit = AI_VALUE2(Unit*, "find target", "sapphiron");
            if (!_unit)
                return false;
        }
        bool now_flying = _unit->IsFlying();
        if (_was_flying && !now_flying)
            _last_land_ms = getMSTime();

        _was_flying = now_flying;
        return true;
    }
    bool IsPhaseGround() { return _unit && !_unit->IsFlying(); }
    bool IsPhaseFlight() { return _unit && _unit->IsFlying(); }
    bool JustLanded()
    {
        if (!_last_land_ms)
            return false;

        return getMSTime() - _last_land_ms <= POSITION_TIME_AFTER_LANDED;
    }
    bool WaitForExplosion()
    {
        if (!IsPhaseFlight())
            return false;

        Group* group = bot->GetGroup();
        if (!group)
            return false;

        for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
        {
            Player* member = ref->GetSource();
            if (member &&
                (NaxxSpellIds::HasAnyAura(member, {NaxxSpellIds::Icebolt10, NaxxSpellIds::Icebolt25}) ||
                 botAI->HasAura("icebolt", member, false, false, -1, true)))
            {
                return true;
            }
        }
        return false;
    }
    bool FindPosToAvoidChill(std::vector<float>& dest)
    {
        Aura* aura = NaxxSpellIds::GetAnyAura(bot, {NaxxSpellIds::Chill10, NaxxSpellIds::Chill25});
        if (!aura)
        {
            // Fallback to name for custom spell data.
            aura = botAI->GetAura("chill", bot);
        }
        if (!aura)
            return false;

        DynamicObject* dyn_obj = aura->GetDynobjOwner();
        if (!dyn_obj)
            return false;

        Unit* currentTarget = AI_VALUE(Unit*, "current target");
        float angle = 0;
        uint32 index = botAI->GetGroupSlotIndex(bot);
        if (currentTarget)
        {
            if (botAI->IsRanged(bot))
            {
                if (bot->GetExactDist2d(currentTarget) <= 45.0f)
                    angle = bot->GetAngle(dyn_obj) - M_PI + (rand_norm() - 0.5) * M_PI / 2;
                else
                {
                    if (index % 2 == 0)
                        angle = bot->GetAngle(currentTarget) + M_PI / 2;
                    else
                        angle = bot->GetAngle(currentTarget) - M_PI / 2;
                }
            }
            else
            {
                if (index % 3 == 0)
                    angle = bot->GetAngle(currentTarget);
                else if (index % 3 == 1)
                    angle = bot->GetAngle(currentTarget) + M_PI / 2;
                else
                    angle = bot->GetAngle(currentTarget) - M_PI / 2;
            }
        }
        else
            angle = bot->GetAngle(dyn_obj) - M_PI + (rand_norm() - 0.5) * M_PI / 2;

        dest = {bot->GetPositionX() + cos(angle) * 5.0f, bot->GetPositionY() + sin(angle) * 5.0f, bot->GetPositionZ()};
        return true;
    }

private:
    void Reset()
    {
        _unit = nullptr;
        _was_flying = false;
        _last_land_ms = 0;
    }

    const uint32 POSITION_TIME_AFTER_LANDED = 5000;
    Unit* _unit = nullptr;
    bool _was_flying = false;
    uint32 _last_land_ms = 0;
};

class GluthBossHelper : public AiObject
{
public:
    const std::pair<float, float> mainTankPos25 = /* Reizan: upstream's 25-man spot was 67y off Gluth's platform and the
   MT walked into the alcove every pull; use the verified platform spot
   (same as 10-man). */ {3278.29f, -3162.06f};
    const std::pair<float, float> mainTankPos10 = {3278.29f, -3162.06f};
    const std::pair<float, float> beforeDecimatePos = {3267.34f, -3175.68f};
    const std::pair<float, float> leftSlowDownPos = {3290.68f, -3141.65f};
    const std::pair<float, float> rightSlowDownPos = {3300.78f, -3151.98f};
    const std::pair<float, float> rangedPos = {3301.45f, -3139.29f};
    const std::pair<float, float> healPos = {3303.09f, -3135.24f};

    const float decimatedZombiePct = 10.0f;
    GluthBossHelper(PlayerbotAI* botAI) : AiObject(botAI) {}
    bool UpdateBossAI()
    {
        if (!bot->IsInCombat())
            Reset();

        if (_unit && (!_unit->IsInWorld() || !_unit->IsAlive()))
            Reset();

        if (!_unit)
        {
            _unit = AI_VALUE2(Unit*, "find target", "gluth");
            if (!_unit)
                return false;
        }
        if (_unit->IsInCombat())
        {
            if (_combat_start_ms == 0)
                _combat_start_ms = getMSTime();
        }
        else
            _combat_start_ms = 0;

        return true;
    }
    bool BeforeDecimate()
    {
        if (!_unit || !_unit->HasUnitState(UNIT_STATE_CASTING))
            return false;

        Spell* spell = _unit->GetCurrentSpell(CURRENT_GENERIC_SPELL);
        if (!spell)
            spell = _unit->GetCurrentSpell(CURRENT_CHANNELED_SPELL);

        if (!spell)
            return false;

        SpellInfo const* info = spell->GetSpellInfo();
        if (!info)
            return false;

        if (NaxxSpellIds::MatchesAnySpellId(
                info, {NaxxSpellIds::Decimate10, NaxxSpellIds::Decimate25, NaxxSpellIds::Decimate25Alt}))
            return true;

        // Fallback to name for custom spell data.
        return info->SpellName[LOCALE_enUS] && botAI->EqualLowercaseName(info->SpellName[LOCALE_enUS], "decimate");
    }
    bool JustStartCombat() const { return _combat_start_ms != 0 && getMSTime() - _combat_start_ms < 10000; }
    bool IsZombieChow(Unit* unit) const { return unit && botAI->EqualLowercaseName(unit->GetName(), "zombie chow"); }

private:
    void Reset()
    {
        _unit = nullptr;
        _combat_start_ms = 0;
    }

    Unit* _unit = nullptr;
    uint32 _combat_start_ms = 0;
};

class LoathebBossHelper : public AiObject
{
public:
    const std::pair<float, float> mainTankPos = {2877.57f, -3967.00f};
    const std::pair<float, float> rangePos = {2896.96f, -3980.61f};
    LoathebBossHelper(PlayerbotAI* botAI) : AiObject(botAI) {}
    bool UpdateBossAI()
    {
        if (!bot->IsInCombat())
            Reset();

        if (_unit && (!_unit->IsInWorld() || !_unit->IsAlive()))
            Reset();

        if (!_unit)
            _unit = AI_VALUE2(Unit*, "find target", "loatheb");

        return _unit != nullptr;
    }

private:
    void Reset() { _unit = nullptr; }

    Unit* _unit = nullptr;
};

class FourHorsemenBossHelper : public AiObject
{
public:
    const float posZ = 241.27f;
    const std::pair<float, float> attractPos[2] = {{2502.03f, -2910.90f},
                                                   {2484.61f, -2947.07f}};  // left (sir zeliek), right (lady blaumeux)
    FourHorsemenBossHelper(PlayerbotAI* botAI) : AiObject(botAI) {}
    bool UpdateBossAI()
    {
        if (!bot->IsInCombat())
            Reset();

        else if (_combat_start_ms == 0)
            _combat_start_ms = getMSTime();

        if (_sir && (!_sir->IsInWorld() || !_sir->IsAlive()))
            Reset();

        if (!_sir)
        {
            // Anchor on ANY surviving horseman — keying on Zeliek alone
            // froze all boss logic the moment he died mid-fight.
            for (char const* name :
                 {"sir zeliek", "thane korth'azz", "lady blaumeux", "baron rivendare", "highlord mograine"})
            {
                _sir = AI_VALUE2(Unit*, "find target", name);
                if (_sir)
                    break;
            }
            if (!_sir)
                return false;
        }
        _lady = AI_VALUE2(Unit*, "find target", "lady blaumeux");
        return true;
    }
    void Reset()
    {
        _sir = nullptr;
        _lady = nullptr;
        _combat_start_ms = 0;
        posToGo = 0;
    }
    bool IsAttracter(Player* bot)
    {
        Difficulty diff = bot->GetRaidDifficulty();
        if (diff == RAID_DIFFICULTY_25MAN_NORMAL)
        {
            return botAI->IsAssistRangedDpsOfIndex(bot, 0) || botAI->IsAssistHealOfIndex(bot, 0) ||
                   botAI->IsAssistHealOfIndex(bot, 1) || botAI->IsAssistHealOfIndex(bot, 2);
        }
        return botAI->IsAssistRangedDpsOfIndex(bot, 0) || botAI->IsAssistHealOfIndex(bot, 0);
    }
    void CalculatePosToGo(Player* bot)
    {
        bool raid25 = bot->GetRaidDifficulty() == RAID_DIFFICULTY_25MAN_NORMAL;
        Unit* lady = _lady;
        if (!lady)
            posToGo = 0;
        else
        {
            uint32 elapsed_ms = _combat_start_ms ? getMSTime() - _combat_start_ms : 0;
            // Interval: 24s - 15s - 15s - ...
            // Swap corners every 45s (~3-4 Mark applications at the ~12s
            // cadence). Upstream's 9s offset + 67.5s half-period drifted far
            // out of sync with the stacks it exists to shed.
            posToGo = (elapsed_ms / 45000) % 2;
            if (botAI->IsAssistRangedDpsOfIndex(bot, 0) || (raid25 && botAI->IsAssistHealOfIndex(bot, 1)))
                posToGo = 1 - posToGo;
        }
    }
    std::pair<float, float> CurrentAttractPos()
    {
        bool raid25 = bot->GetRaidDifficulty() == RAID_DIFFICULTY_25MAN_NORMAL;
        float posX = attractPos[posToGo].first, posY = attractPos[posToGo].second;
        if (posToGo == 1)
        {
            float offset_x = 0.0f;
            float offset_y = 0.0f;
            float bias = 4.5f;
            if (raid25)
            {
                offset_x = -bias;
                offset_y = bias;
            }
            posX += offset_x;
            posY += offset_y;
        }
        return {posX, posY};
    }
    Unit* CurrentAttackTarget()
    {
        if (posToGo == 0)
            return _sir;

        return _lady;
    }

protected:
    Unit* _sir = nullptr;
    Unit* _lady = nullptr;
    uint32 _combat_start_ms = 0;
    int posToGo = 0;
};
class ThaddiusBossHelper : public AiObject
{
public:
    // Tesla tether: a pet more than 28y from its spawn breaks its coil link
    // and the coil shocks random raiders for ~4.4k every 1.5s. The pet walks
    // to whoever tanks it, so these sit close to the spawns (Stalagg
    // 3450.45/-2931.42, Feugen 3508.14/-2988.65) — same direction as the old
    // spots, ~7y out instead of ~19y, leaving the tether budget untouched.
    const std::pair<float, float> tankPosFeugen = {3514.06f, -2994.23f};
    const std::pair<float, float> tankPosStalagg = {3444.73f, -2926.84f};
    const std::pair<float, float> rangedPosFeugen = {3500.45f, -2997.92f};
    const std::pair<float, float> rangedPosStalagg = {3441.01f, -2942.04f};
    const float tankPosZ = 312.61f;
    ThaddiusBossHelper(PlayerbotAI* botAI) : AiObject(botAI) {}
    bool UpdateBossAI()
    {
        if (!bot->IsInCombat())
            Reset();

        if (_unit && (!_unit->IsInWorld() || !_unit->IsAlive()))
            Reset();

        if (!_unit)
            _unit = AI_VALUE2(Unit*, "find target", "thaddius");

        feugen = AI_VALUE2(Unit*, "find target", "feugen");
        stalagg = AI_VALUE2(Unit*, "find target", "stalagg");

        // The pets anchor the whole first phase: Thaddius holds no threat
        // while Stalagg and Feugen live, so gating on resolving HIM made
        // every trigger, tank split, and the death-sync multiplier inert
        // until phase two — the fight fell apart before boss AI ever ran.
        //
        // Sticky once engaged: between the pets dying and Thaddius picking
        // up threat, ALL THREE resolve to nothing, and a plain check would
        // switch the encounter AI off during exactly the window the raid
        // needs it — the jump to his platform. Cleared by Reset() when
        // combat ends.
        if (_unit || feugen || stalagg)
            _engaged = true;

        return _engaged && bot->IsInCombat();
    }
    bool IsPhasePet() { return (feugen && feugen->IsAlive()) || (stalagg && stalagg->IsAlive()); }
    bool IsPhaseTransition()
    {
        if (IsPhasePet())
            return false;

        // Pets down but Thaddius not yet on our threat list (he has not
        // engaged), or still flagged/immune (IP-40 uses PC-immunity rather
        // than the wotlk non-attackable flag): jump-across time.
        if (!_unit)
            return true;

        return _unit->HasUnitFlag(UNIT_FLAG_NON_ATTACKABLE) || _unit->IsImmuneToPC();
    }
    bool IsPhaseThaddius() { return !IsPhasePet() && !IsPhaseTransition(); }
    Unit* GetNearestPet()
    {
        Unit* unit = nullptr;
        if (feugen && feugen->IsAlive())
            unit = feugen;

        if (stalagg && stalagg->IsAlive() &&
            (!feugen || !feugen->IsAlive() || bot->GetDistance(stalagg) < bot->GetDistance(feugen)))
            unit = stalagg;

        return unit;
    }

    // Assigned split — 'nearest pet' breaks at the pull, when the whole
    // raid stands in one clump and everyone's nearest pet is the same pet.
    // Main tank and even ranks take Stalagg, first assist tank and odd
    // ranks take Feugen; a dead pet folds its team onto the survivor.
    Unit* GetAssignedPet(Player* forBot)
    {
        bool const stalaggAlive = stalagg && stalagg->IsAlive();
        bool const feugenAlive = feugen && feugen->IsAlive();
        if (!stalaggAlive)
            return feugenAlive ? feugen : nullptr;
        if (!feugenAlive)
            return stalagg;

        // Has the raid already taken sides? Count living bots parked near
        // each pet. If BOTH camps are meaningfully populated, the leader
        // pre-positioned (or the fight is already running), so honour where
        // people actually stand — otherwise a manual split with goto/sweep
        // gets shuffled away by the parity rule below and everyone sprints
        // across the room at the pull. A clump means nobody has split yet,
        // and parity divides it. Every bot counts the same roster, so all
        // forty reach the same verdict.
        uint32 nearStalagg = 0, nearFeugen = 0, total = 0;
        if (Group* group = forBot->GetGroup())
            for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
            {
                Player* member = itr->GetSource();
                if (!member || !member->IsAlive() || !GET_PLAYERBOT_AI(member))
                    continue;

                ++total;
                if (member->GetDistance(stalagg) < 40.0f)
                    ++nearStalagg;
                else if (member->GetDistance(feugen) < 40.0f)
                    ++nearFeugen;
            }

        if (total >= 4 && nearStalagg * 4 >= total && nearFeugen * 4 >= total)
            return forBot->GetDistance(stalagg) < forBot->GetDistance(feugen) ? stalagg : feugen;

        if (botAI->IsMainTank(forBot))
            return stalagg;
        if (PlayerbotAI::IsAssistTankOfIndex(forBot, 0))
            return feugen;

        uint32 rank = 0;
        if (Group* group = forBot->GetGroup())
            for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
            {
                Player* member = itr->GetSource();
                if (!member || !member->IsAlive() || !GET_PLAYERBOT_AI(member))
                    continue;

                if (member == forBot)
                    break;

                ++rank;
            }

        return (rank % 2 == 0) ? stalagg : feugen;
    }

    // The pet this bot is actually tanking right now. Stalagg's Magnetic
    // Pull swaps the two tanks every 20s (teleport + threat transfer), so a
    // tank's static assignment goes stale mid-fight; whatever he holds is
    // his, or he drags it across the room and snaps the tether.
    Unit* GetHeldPet(Player* forBot)
    {
        if (stalagg && stalagg->IsAlive() && stalagg->GetVictim() == forBot)
            return stalagg;

        if (feugen && feugen->IsAlive() && feugen->GetVictim() == forBot)
            return feugen;

        return nullptr;
    }

    Unit* GetTankPet(Player* forBot)
    {
        if (Unit* held = GetHeldPet(forBot))
            return held;

        return GetAssignedPet(forBot);
    }

    std::pair<float, float> PetPhaseGetPosForTank()
    {
        if (GetTankPet(bot) == feugen)
            return tankPosFeugen;

        return tankPosStalagg;
    }
    std::pair<float, float> PetPhaseGetPosForRanged()
    {
        if (GetAssignedPet(bot) == feugen)
            return rangedPosFeugen;

        return rangedPosStalagg;
    }

protected:
    void Reset()
    {
        _unit = nullptr;
        feugen = nullptr;
        stalagg = nullptr;
        _engaged = false;
    }

    Unit* _unit = nullptr;
    Unit* feugen = nullptr;
    Unit* stalagg = nullptr;
    bool _engaged = false;
};

#endif

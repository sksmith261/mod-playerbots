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
// ---- Four Horsemen ----------------------------------------------------
// One camp per horseman, taken from the corner waypoints of the same
// physical room. Each horseman's Mark is an aura pulsed over roughly 45y,
// so camps this far apart mean a raider only ever collects one kind.
struct HorsemanSpec
{
    char const* name;
    char const* altName;   // IP renames Rivendare to Highlord Mograine
    uint32 markId;
    float x, y;
};

inline HorsemanSpec const* FourHorsemenSpecs()
{
    static HorsemanSpec const specs[4] = {
        {"thane korth'azz",   nullptr,           28832, 2542.9f, -3015.0f},  // Meteor: stack to split it
        {"highlord mograine", "baron rivendare", 28834, 2583.9f, -2971.6f},  // Unholy Shadow: nothing special
        {"lady blaumeux",     nullptr,           28833, 2471.76f, -2948.9f}, // Void Zone: keep moving
        {"sir zeliek",        nullptr,           28835, 2514.57f, -2899.91f}, // Holy Wrath: ranged, spread wide
    };
    return specs;
}

// Centroid of the four camps: 57-62y from every one of them, so it sits
// outside all four Mark radii at once. This is where a tank rotated off
// its horseman waits for its stacks to fall away.
constexpr float FH_SAFE_X = 2528.5f;
constexpr float FH_SAFE_Y = -2957.7f;
// Mark damage lands on each application, scaled to the stacks at that
// moment: 0, 250, 1000, then 3000 at the fourth. Swapping AT four means
// the tank has already taken the 3000 — so the handover starts at three,
// capping a tank's whole rotation at about 1250 instead.
constexpr uint32 FH_SWAP_STACKS = 3;

inline bool IsHorseman(PlayerbotAI* botAI, Unit* unit)
{
    if (!unit)
        return false;

    for (uint32 i = 0; i < 4; ++i)
    {
        HorsemanSpec const& spec = FourHorsemenSpecs()[i];
        if (botAI->EqualLowercaseName(unit->GetName(), spec.name))
            return true;
        if (spec.altName && botAI->EqualLowercaseName(unit->GetName(), spec.altName))
            return true;
    }

    return false;
}

inline Unit* ResolveHorseman(PlayerbotAI* botAI, HorsemanSpec const& spec)
{
    Unit* unit = botAI->GetAiObjectContext()->GetValue<Unit*>("find target", spec.name)->Get();
    if (!unit && spec.altName)
        unit = botAI->GetAiObjectContext()->GetValue<Unit*>("find target", spec.altName)->Get();

    // Threat shows only what this bot personally fights, and at the pull
    // nobody holds any of the four — so every camp assignment resolved to
    // nothing and the raid simply stood still. Grid fallback, in combat
    // only, so camps can be taken before a blow is struck.
    Player* bot = botAI->GetBot();
    if (!unit && bot && bot->IsInCombat())
    {
        for (auto const& guid : botAI->GetAiObjectContext()->GetValue<GuidVector>("possible targets no los")->Get())
        {
            Unit* candidate = botAI->GetUnit(guid);
            if (!candidate)
                continue;

            if (botAI->EqualLowercaseName(candidate->GetName(), spec.name) ||
                (spec.altName && botAI->EqualLowercaseName(candidate->GetName(), spec.altName)))
            {
                unit = candidate;
                break;
            }
        }
    }

    return (unit && unit->IsAlive()) ? unit : nullptr;
}

// Anyone who can hold a boss — dedicated tanks first, then damage specs of
// the classes that can: three real tanks cannot cover four horsemen, so
// the pool is padded with promotable damage until it reaches eight, which
// is two per horseman and therefore one swap partner each.
inline bool CanHoldHorseman(Player* p)
{
    if (PlayerbotAI::IsTank(p))
        return true;

    // Never conscript a healer. Paladin and druid are both on the list
    // below, so holy and restoration specs were being promoted — handed a
    // horseman to hold in healing gear, and taken off healing at the same
    // time. With three real tanks the first promotion lands on Blaumeux,
    // which is exactly where this kept going wrong.
    if (PlayerbotAI::IsHeal(p))
        return false;

    switch (p->getClass())
    {
        case CLASS_WARRIOR:
        case CLASS_PALADIN:
        case CLASS_DRUID:
        case CLASS_DEATH_KNIGHT:
            return true;
        default:
            return false;
    }
}

inline std::vector<Player*> FourHorsemenTankPool(Player* bot)
{
    // Promotion order: real tanks, death knights, paladins, warriors,
    // druids. Death knights and paladins taunt as they stand; warriors and
    // druids must shift into Defensive Stance or Bear Form first, which the
    // taunt helper handles but which costs a tick on every re-taunt — and
    // on this fight re-taunting is constant, because each Mark halves the
    // tank's threat.
    std::vector<Player*> real, deathKnights, warriors, paladins, druids;
    if (Group* group = bot->GetGroup())
        for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
        {
            Player* member = itr->GetSource();
            if (!member || !member->IsAlive() || !GET_PLAYERBOT_AI(member) || !CanHoldHorseman(member))
                continue;

            if (PlayerbotAI::IsTank(member))
                real.push_back(member);
            else
                switch (member->getClass())
                {
                    case CLASS_DEATH_KNIGHT: deathKnights.push_back(member); break;
                    case CLASS_WARRIOR:      warriors.push_back(member);     break;
                    case CLASS_PALADIN:      paladins.push_back(member);     break;
                    default:                 druids.push_back(member);       break;
                }
        }

    real.insert(real.end(), deathKnights.begin(), deathKnights.end());
    real.insert(real.end(), paladins.begin(), paladins.end());
    real.insert(real.end(), warriors.begin(), warriors.end());
    real.insert(real.end(), druids.begin(), druids.end());
    if (real.size() > 8)
        real.resize(8);

    return real;
}

// Pool slots k and k+4 share horseman k. Whichever of the two is under the
// swap threshold holds it; if both are over, the lighter one does.
inline Player* FourHorsemenActiveTank(std::vector<Player*> const& pool, uint32 group, uint32 markId, Unit* boss)
{
    auto stacksOf = [&](Player* p) -> uint32
    {
        Aura* mark = p->GetAura(markId);
        return mark ? mark->GetStackAmount() : 0u;
    };

    // Whoever holds it keeps it until they reach the threshold. Without
    // this the election simply returns the lowest-numbered candidate under
    // the limit, so the boss is handed straight back the moment the rested
    // partner's stacks lapse and the pair trade it every few seconds.
    if (boss)
        for (uint32 i = group; i < pool.size(); i += 4)
            if (boss->GetVictim() == pool[i] && stacksOf(pool[i]) < FH_SWAP_STACKS)
                return pool[i];

    Player* best = nullptr;
    uint32 bestStacks = 0;

    for (uint32 i = group; i < pool.size(); i += 4)
    {
        uint32 const stacks = stacksOf(pool[i]);
        if (stacks < FH_SWAP_STACKS)
            return pool[i];

        if (!best || stacks < bestStacks)
        {
            best = pool[i];
            bestStacks = stacks;
        }
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

// boss_gothik_40.cpp: IN_LIVE_SIDE(who) is (y < POS_Y_GATE). The inner
// gate only opens at 30% boss health, so until then a bot cannot reach
// anything on the far side — targeting across it just walks it into a
// closed gate.
constexpr float GOTHIK_GATE_Y = -3360.78f;

inline bool GothikLiveSide(WorldObject const* who) { return who->GetPositionY() < GOTHIK_GATE_Y; }

// Threat first, then a grid scan: Gothik holds no threat on most of the
// raid during the wave phase, and the bots still need to know where he is.
inline Unit* FindGothik(PlayerbotAI* botAI, Player* bot)
{
    if (Unit* boss = botAI->GetAiObjectContext()->GetValue<Unit*>("find target", "gothik the harvester")->Get())
        return boss;

    if (!bot->IsInCombat())
        return nullptr;

    for (auto const& guid : botAI->GetAiObjectContext()->GetValue<GuidVector>("possible targets no los")->Get())
    {
        Unit* unit = botAI->GetUnit(guid);
        if (unit && botAI->EqualLowercaseName(unit->GetName(), "gothik the harvester"))
            return unit;
    }

    return nullptr;
}

// Wave phase: the script keeps him REACT_PASSIVE on his balcony and only
// flips him aggressive when the last wave has been sent. Reading his react
// state beats guessing from altitude.
inline bool GothikWavePhase(Unit* gothik)
{
    Creature* creature = gothik ? gothik->ToCreature() : nullptr;
    return creature && creature->HasReactState(REACT_PASSIVE);
}

// Below 30% the script opens the inner gate and the two sides merge.
inline bool GothikGateOpen(Unit* gothik) { return gothik && gothik->GetHealthPct() < 30.0f; }

// Gothik wave adds; higher rank dies first. 0 = not a Gothik add.
inline int32 GothikAddRank(PlayerbotAI* botAI, Unit* unit)
{
    std::string name = unit->GetName();
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    if (name.find("rider") != std::string::npos && name.find("unrelenting") == 0)
        return 3;
    if (name == "spectral rider")
        return 3;
    // IP names these "Unrelenting Deathknight" / "Spectral Deathknight" —
    // one word. Matching only the wotlk "death knight" spelling meant the
    // whole middle tier of the waves was invisible to the kill order.
    if ((name.find("deathknight") != std::string::npos || name.find("death knight") != std::string::npos) &&
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
    // Loatheb does not move and has no positional mechanic — he is a
    // healing-throughput fight. The old tank spot sat 43.7y from his spawn,
    // which meant dragging him most of the way across the room to start.
    // Tank him where he stands; ranged keep their spot, which is a
    // reasonable 20y out.
    std::pair<float, float> mainTankPos = {2909.00f, -3997.41f};
    const std::pair<float, float> rangePos = {2896.96f, -3980.61f};
    LoathebBossHelper(PlayerbotAI* botAI) : AiObject(botAI) {}

    // Prefer his live spawn if we can see him, so this cannot drift.
    void AnchorPositions()
    {
        if (_unit)
            if (Creature* creature = _unit->ToCreature())
            {
                mainTankPos.first = creature->GetHomePosition().GetPositionX();
                mainTankPos.second = creature->GetHomePosition().GetPositionY();
            }
    }
    bool UpdateBossAI()
    {
        if (!bot->IsInCombat())
            Reset();

        if (_unit && (!_unit->IsInWorld() || !_unit->IsAlive()))
            Reset();

        if (!_unit)
            _unit = AI_VALUE2(Unit*, "find target", "loatheb");

        AnchorPositions();
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

            // Threat only shows what this bot personally fights, and most
            // of the raid holds none of the four. Same failure that left
            // Thaddius inert: fall back to a grid scan so everyone agrees
            // the encounter is running.
            if (!_sir && bot->IsInCombat())
            {
                for (auto const& guid : AI_VALUE(GuidVector, "possible targets no los"))
                {
                    Unit* unit = botAI->GetUnit(guid);
                    if (!unit || !unit->IsAlive())
                        continue;

                    for (char const* name : {"sir zeliek", "thane korth'azz", "lady blaumeux",
                                             "baron rivendare", "highlord mograine"})
                        if (botAI->EqualLowercaseName(unit->GetName(), name))
                        {
                            _sir = unit;
                            break;
                        }

                    if (_sir)
                        break;
                }
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
    // Damage is held for the opening seconds so four tanks can pull four
    // horsemen out to four camps and build real threat. Marks halve tank
    // threat constantly, so a raid that opens up mid-drag rips them loose
    // and they end up piled in the middle.
    bool InPullGrace() const
    {
        return _combat_start_ms != 0 && getMSTime() - _combat_start_ms < 10000;
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
    // Stations are computed, not hardcoded, and always lie ON THE SEGMENT
    // between a pet's spawn point and its platform's jump ledge — the only
    // two points in this room known to sit on the platform at its height.
    //
    // The old fixed coordinates pointed the other way: from each pet the
    // ledge lies at bearing ~+44 degrees, while the tank and ranged spots
    // sat at -43, -130, +141 and -132. Those are over the platform edge,
    // above the slime floor ~17y below, so ranged walked down into the pool
    // and fought "from the middle", and Feugen's tank dragged him
    // underneath the platform into the water.
    const std::pair<float, float> ledgeStalagg = {3462.99f, -2918.90f};
    const std::pair<float, float> ledgeFeugen = {3520.65f, -2976.51f};

    // Set from the pet's own spawn height whenever a station is computed,
    // so nothing depends on a hardcoded floor level either.
    float tankPosZ = 312.09f;

    std::pair<float, float> PetStation(Unit* pet, float outDistance)
    {
        Creature* creature = pet ? pet->ToCreature() : nullptr;
        if (!creature)
            return {bot->GetPositionX(), bot->GetPositionY()};  // no-op move

        Position const& home = creature->GetHomePosition();
        tankPosZ = home.GetPositionZ();

        std::pair<float, float> const& ledge =
            botAI->EqualLowercaseName(pet->GetName(), "stalagg") ? ledgeStalagg : ledgeFeugen;

        float const dx = ledge.first - home.GetPositionX();
        float const dy = ledge.second - home.GetPositionY();
        float const len = std::sqrt(dx * dx + dy * dy);
        if (len < 1.0f)
            return {home.GetPositionX(), home.GetPositionY()};

        return {home.GetPositionX() + dx / len * outDistance,
                home.GetPositionY() + dy / len * outDistance};
    }
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

        // "find target" only sees what THIS bot personally holds threat on.
        // A healer before its first heal, a bot that just rezzed, anyone who
        // dropped off a pet's threat list — all of them saw no pets, decided
        // the pet phase was over, and ran for Thaddius's platform, straight
        // across the slime. It also silently disabled their leash guard, so
        // a tank in that state dragged its pet into the pool with it.
        //
        // Phase state must not be a per-bot opinion: fall back to a grid
        // scan so all forty agree. In-combat only, so it cannot body-pull.
        if (bot->IsInCombat() && (!feugen || !stalagg))
        {
            for (auto const& guid : AI_VALUE(GuidVector, "possible targets no los"))
            {
                Unit* unit = botAI->GetUnit(guid);
                if (!unit || !unit->IsAlive())
                    continue;

                if (!feugen && botAI->EqualLowercaseName(unit->GetName(), "feugen"))
                    feugen = unit;
                else if (!stalagg && botAI->EqualLowercaseName(unit->GetName(), "stalagg"))
                    stalagg = unit;
            }
        }

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

    // 3y keeps the pet within a couple of yards of its spawn; 12y puts
    // ranged outside Feugen's Static Field but still short of the ledge.
    std::pair<float, float> PetPhaseGetPosForTank() { return PetStation(GetTankPet(bot), 3.0f); }
    std::pair<float, float> PetPhaseGetPosForRanged() { return PetStation(GetAssignedPet(bot), 12.0f); }

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

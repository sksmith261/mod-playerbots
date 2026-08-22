#include "NaxxRaidPlans.h"

#include <algorithm>

#include "NaxxBossHelper.h"
#include "GameObject.h"
#include "ObjectAccessor.h"
#include "Playerbots.h"
#include "Timer.h"

namespace
{
using namespace NaxxHelpers;

constexpr uint32 PULL_GRACE_MS = 10000;

// Perimeter order of the four camps and the diagonal opposite of each.
// Measured: Korth'azz-Mograine 60y and Blaumeux-Zeliek 66y are the short
// sides, Korth'azz-Blaumeux and Mograine-Zeliek 100y the long ones, and
// Korth'azz-Zeliek / Mograine-Blaumeux 117-119y the diagonals.
constexpr uint32 RING[4] = {0u, 1u, 3u, 2u};
constexpr uint32 DIAGONAL[4] = {3u, 2u, 1u, 0u};

uint32 StacksOf(Player* player, uint32 markId)
{
    Aura* mark = player->GetAura(markId);
    return mark ? mark->GetStackAmount() : 0u;
}

// Promotion order: real tanks, then death knights, paladins, warriors,
// druids. Death knights and paladins taunt as they stand; warriors and
// druids must shift into Defensive Stance or Bear Form first, and on this
// fight re-taunting is constant because every Mark halves tank threat.
uint32 PromotionRank(Player* player)
{
    if (PlayerbotAI::IsTank(player))
        return 0;

    switch (player->getClass())
    {
        case CLASS_DEATH_KNIGHT: return 1;
        case CLASS_PALADIN:      return 2;
        case CLASS_WARRIOR:      return 3;
        case CLASS_DRUID:        return 4;
        default:                 return 5;  // cannot hold a boss
    }
}
}  // namespace

bool NaxxRaidPlans::BuildFourHorsemen(Player* bot, Group* group, RaidPlan& plan)
{
    PlayerbotAI* botAI = GET_PLAYERBOT_AI(bot);
    if (!botAI || bot->GetMapId() != NAXX_MAP_ID)
        return false;

    HorsemanSpec const* specs = FourHorsemenSpecs();

    // Perception, once, from one vantage point. Every bot afterwards reads
    // GUIDs out of the plan instead of asking its own threat list, which is
    // the defect that left most of the raid outside the encounter AI.
    Unit* horsemen[4] = {};
    bool any = false;
    for (uint32 i = 0; i < 4; ++i)
    {
        horsemen[i] = ResolveHorseman(botAI, specs[i]);
        any = any || horsemen[i] != nullptr;
    }

    if (!any || !bot->IsInCombat())
        return false;

    std::unordered_map<ObjectGuid, RaidAssignment> const previous = plan.assignments;
    plan.assignments.clear();
    plan.encounter = RAID_ENCOUNTER_FOUR_HORSEMEN;

    if (!plan.engagedMs)
        plan.engagedMs = getMSTime();

    bool const grace = getMSTime() - plan.engagedMs < PULL_GRACE_MS;
    plan.phase = grace ? 1u : 2u;

    // ---- Roster ----------------------------------------------------------
    std::vector<Player*> pool, healers, melee, ranged;
    for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
    {
        Player* member = itr->GetSource();
        if (!member || !member->IsAlive() || !GET_PLAYERBOT_AI(member))
            continue;

        if (PlayerbotAI::IsHeal(member))
        {
            healers.push_back(member);
            continue;  // never conscript a healer as a tank
        }

        if (PromotionRank(member) < 5)
            pool.push_back(member);

        (PlayerbotAI::IsRanged(member) ? ranged : melee).push_back(member);
    }

    std::stable_sort(pool.begin(), pool.end(),
                     [](Player* a, Player* b) { return PromotionRank(a) < PromotionRank(b); });

    // ---- Tanks: one per camp, with hysteresis ----------------------------
    // Real tanks cover Korth'azz, Mograine and Zeliek; the promoted spec
    // gets Blaumeux, who only casts and is the least punishing to hold.
    static uint32 const campForPoolSlot[4] = {0u, 1u, 3u, 2u};
    std::unordered_map<ObjectGuid, bool> taken;
    Player* holders[4] = {};

    for (uint32 slot = 0; slot < 4; ++slot)
    {
        uint32 const camp = campForPoolSlot[slot];
        if (!horsemen[camp])
            continue;

        uint32 const markId = specs[camp].markId;

        // Whoever held this camp last tick keeps it until their own stacks
        // reach the threshold. Without that the choice flips back the moment
        // a rested partner's stacks lapse and the pair trade the boss every
        // few seconds.
        for (auto const& [guid, old] : previous)
        {
            if (old.duty != RAID_DUTY_TANK || old.camp != camp)
                continue;

            Player* previousHolder = ObjectAccessor::FindPlayer(guid);
            if (previousHolder && previousHolder->IsAlive() && !taken.count(guid) &&
                StacksOf(previousHolder, markId) < FH_SWAP_STACKS)
            {
                holders[camp] = previousHolder;
                taken[guid] = true;
            }
            break;
        }

        if (holders[camp])
            continue;

        // Otherwise the lightest-loaded eligible bot in promotion order.
        for (Player* candidate : pool)
        {
            if (taken.count(candidate->GetGUID()))
                continue;

            if (StacksOf(candidate, markId) >= FH_SWAP_STACKS)
                continue;

            holders[camp] = candidate;
            taken[candidate->GetGUID()] = true;
            break;
        }
    }

    for (uint32 camp = 0; camp < 4; ++camp)
        if (holders[camp])
            plan.assignments[holders[camp]->GetGUID()] = {
                RAID_DUTY_TANK, camp, horsemen[camp] ? horsemen[camp]->GetGUID() : ObjectGuid::Empty};

    // A pool member that is not holding anything and still carries stacks is
    // a reserve: it waits at the safe spot and must not re-engage.
    for (Player* member : pool)
    {
        if (plan.assignments.count(member->GetGUID()))
            continue;

        for (uint32 camp = 0; camp < 4; ++camp)
            if (StacksOf(member, specs[camp].markId) > 0)
            {
                plan.assignments[member->GetGUID()] = {
                    RAID_DUTY_RESERVE, camp,
                    horsemen[camp] ? horsemen[camp]->GetGUID() : ObjectGuid::Empty};
                break;
            }
    }

    // ---- Camp rotation for everyone else ---------------------------------
    // A camp is a commitment carried in the plan, not something re-derived
    // from where a bot currently stands: deriving it that way flipped the
    // answer as a bot crossed the midpoint, and it turned round in the open.
    auto assignRotating = [&](Player* member, uint32 campA, uint32 campB, uint8 duty)
    {
        if (plan.assignments.count(member->GetGUID()))
            return;

        uint32 camp = campA;
        if (auto it = previous.find(member->GetGUID()); it != previous.end())
            if (it->second.camp == campA || it->second.camp == campB)
                camp = it->second.camp;

        if (StacksOf(member, specs[camp].markId) >= FH_SWAP_STACKS)
            camp = (camp == campA) ? campB : campA;

        if (!horsemen[camp])
            camp = (camp == campA) ? campB : campA;

        plan.assignments[member->GetGUID()] = {
            duty, camp, horsemen[camp] ? horsemen[camp]->GetGUID() : ObjectGuid::Empty};
    };

    // Healers rotate clockwise — the short hop round the perimeter. Stepping
    // by raw camp index instead sends half of them across a 117y diagonal,
    // out of range for seventeen seconds and gathering marks the whole way.
    for (uint32 i = 0; i < healers.size(); ++i)
        assignRotating(healers[i], RING[i % 4], RING[(i % 4 + 1) % 4], RAID_DUTY_HEAL);

    // Ranged rotate diagonally: the long way, which sheds a mark outright
    // instead of trading it for a neighbour's.
    for (uint32 i = 0; i < ranged.size(); ++i)
    {
        uint32 const campA = (i % 2) ? 3u : 2u;
        assignRotating(ranged[i], campA, DIAGONAL[campA], RAID_DUTY_DAMAGE);
    }

    // Melee alternate Korth'azz and Mograine: the shortest hop, and the only
    // two camps that can hold them — Holy Wrath chains through anyone in
    // melee of Zeliek.
    for (uint32 i = 0; i < melee.size(); ++i)
        assignRotating(melee[i], (i % 2) ? 1u : 0u, (i % 2) ? 0u : 1u, RAID_DUTY_DAMAGE);

    return true;
}

// ---------------------------------------------------------------------------
// Sapphiron
//
// Two phases with completely different jobs. On the ground she is tanked and
// the raid spreads — spread matters more than it looks, because the ice
// blocks of the next air phase form wherever people are standing, so a raid
// stacked on the ground has no cover a moment later. In the air she icebolts
// several raiders, drops an ice block gameobject on each, and breathes: every
// bot not encased has to put one of those blocks between itself and her.
//
// Both halves are coordination problems — who is encased, which block each
// bot uses — which is exactly what the plan is for. Sapphiron holds threat on
// almost nobody, so resolving her per-bot was never going to work either.
// ---------------------------------------------------------------------------
namespace
{
constexpr uint32 GO_ICE_BLOCK = 181247;

// Her spawn. Positions are derived from it rather than hardcoded, so nothing
// can sit off the floor the way the old fixed coordinates could.
constexpr float SAPP_X = 3522.39f;
constexpr float SAPP_Y = -5236.78f;

// Ring radius for ranged and healers. Wide enough to spread the ice blocks
// around her, inside spell range of the middle.
constexpr float SAPP_RING = 26.0f;
}  // namespace

bool NaxxRaidPlans::BuildSapphiron(Player* bot, Group* group, RaidPlan& plan)
{
    PlayerbotAI* botAI = GET_PLAYERBOT_AI(bot);
    if (!botAI || bot->GetMapId() != NAXX_MAP_ID || !bot->IsInCombat())
        return false;

    // One resolution for the whole group: threat first, grid otherwise.
    Unit* sapphiron = AI_VALUE2(Unit*, "find target", "sapphiron");
    if (!sapphiron)
        for (auto const& guid : AI_VALUE(GuidVector, "possible targets no los"))
        {
            Unit* unit = botAI->GetUnit(guid);
            if (unit && botAI->EqualLowercaseName(unit->GetName(), "sapphiron"))
            {
                sapphiron = unit;
                break;
            }
        }

    if (!sapphiron || !sapphiron->IsAlive())
        return false;

    plan.assignments.clear();
    plan.encounter = RAID_ENCOUNTER_SAPPHIRON;

    if (!plan.engagedMs)
        plan.engagedMs = getMSTime();

    // The script lifts her with SetDisableGravity(true) and lands her with
    // false, so her own movement state is the phase — no guessing from
    // heights or timers.
    bool const airborne = sapphiron->HasUnitMovementFlag(MOVEMENTFLAG_DISABLE_GRAVITY);
    plan.phase = airborne ? 2u : 1u;

    // Ice blocks are gameobjects dropped on each icebolted raider.
    std::vector<ObjectGuid> blocks;
    if (airborne)
    {
        std::list<GameObject*> found;
        bot->GetGameObjectListWithEntryInGrid(found, GO_ICE_BLOCK, 120.0f);
        for (GameObject* block : found)
            if (block && block->isSpawned())
                blocks.push_back(block->GetGUID());

        std::sort(blocks.begin(), blocks.end());
    }

    uint32 slot = 0, hideSlot = 0;
    for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
    {
        Player* member = itr->GetSource();
        if (!member || !member->IsAlive() || !GET_PLAYERBOT_AI(member))
            continue;

        if (airborne)
        {
            // Encased bots are already safe and cannot act; everyone else is
            // dealt a block, spread evenly so one is not sheltering thirty.
            if (member->HasAura(28522) || member->HasAura(28526))
            {
                plan.assignments[member->GetGUID()] = {RAID_DUTY_HIDE, 0, ObjectGuid::Empty};
                continue;
            }

            ObjectGuid cover = blocks.empty() ? ObjectGuid::Empty : blocks[hideSlot++ % blocks.size()];
            plan.assignments[member->GetGUID()] = {RAID_DUTY_HIDE, 0, cover};
            continue;
        }

        // Ground phase. Tanks hold her essentially on her spawn so she does
        // not get walked around the room; everyone else takes a slot on a
        // ring, which both spreads the raid and scatters the ice blocks that
        // the next air phase will build out of them.
        if (PlayerbotAI::IsTank(member))
        {
            plan.assignments[member->GetGUID()] = {RAID_DUTY_TANK, 0, sapphiron->GetGUID()};
            continue;
        }

        uint8 const duty = PlayerbotAI::IsHeal(member) ? RAID_DUTY_HEAL : RAID_DUTY_DAMAGE;
        bool const ranged = PlayerbotAI::IsRanged(member) || PlayerbotAI::IsHeal(member);
        plan.assignments[member->GetGUID()] = {duty, ranged ? ++slot : 0u, sapphiron->GetGUID()};
    }

    return true;
}

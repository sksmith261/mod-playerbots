#include "NaxxRaidPlans.h"

#include <algorithm>
#include <cstring>

#include "CreatureAI.h"
#include "NaxxBossHelper.h"
#include "GameObject.h"
#include "ObjectAccessor.h"
#include "Playerbots.h"
#include "Timer.h"

namespace
{
using namespace NaxxHelpers;

constexpr uint32 PULL_GRACE_MS = 10000;

// A boss only counts as OUR encounter once somebody in the group has actually
// generated threat on him. IsInCombat() alone was the seam: Patchwerk patrols
// through his own trash, joins their fight by proximity assist the moment the
// raid brawls within his assist range, and "in combat with anyone" then
// declared him the encounter and committed every bot mid-trash-pack. Proximity
// aggro and assists put group members on his threat list at zero; threat above
// zero requires the raid to have fought back, which is the difference between
// "he wandered in" and "we are fighting him". The generic self-defence layer
// still responds if he starts hitting someone, and that response is exactly
// what flips this gate.
bool EngagedWithGroup(Unit* unit, Group* group)
{
    if (!unit->IsInCombat())
        return false;

    for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
    {
        Player* member = itr->GetSource();
        if (member && member->IsAlive() && unit->GetThreatMgr().GetThreat(member) > 0.1f)
            return true;
    }

    return false;
}

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
    plan.label = "Four Horsemen";

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

    // ---- Corners a player already has ------------------------------------
    // The plan cannot drive a player, so the one useful thing it can do about
    // one is not send a bot to fight him for the same horseman. Without this
    // the roster above simply skipped every player in the raid and promoted
    // four bots onto four corners regardless, so two of them piled onto the
    // corner the player was already holding and the raid ran a five-tank
    // rotation on a four-corner fight.
    bool playerHeld[4] = {};
    for (uint32 camp = 0; camp < 4; ++camp)
    {
        if (!horsemen[camp])
            continue;

        Unit* victim = horsemen[camp]->GetVictim();
        Player* holder = victim ? victim->ToPlayer() : nullptr;
        playerHeld[camp] = holder && holder->GetGroup() == group && !GET_PLAYERBOT_AI(holder);
    }

    // ---- Tanks: one per camp, with hysteresis ----------------------------
    // Real tanks cover Korth'azz, Mograine and Zeliek; the promoted spec
    // gets Blaumeux, who only casts and is the least punishing to hold.
    static uint32 const campForPoolSlot[4] = {0u, 1u, 3u, 2u};
    std::unordered_map<ObjectGuid, bool> taken;
    Player* holders[4] = {};

    for (uint32 slot = 0; slot < 4; ++slot)
    {
        uint32 const camp = campForPoolSlot[slot];
        if (!horsemen[camp] || playerHeld[camp])
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
    // Free function: the AI_VALUE macros resolve through a `context` member
    // that only AiObject subclasses have, so go through botAI directly.
    Unit* sapphiron = botAI->GetAiObjectContext()->GetValue<Unit*>("find target", "sapphiron")->Get();
    if (!sapphiron)
        for (auto const& guid :
             botAI->GetAiObjectContext()->GetValue<GuidVector>("possible targets no los")->Get())
        {
            Unit* unit = botAI->GetUnit(guid);
            // Engaged with us only — same through-the-wall reach as the
            // generic sweep.
            if (unit && EngagedWithGroup(unit, group) && botAI->EqualLowercaseName(unit->GetName(), "sapphiron"))
            {
                sapphiron = unit;
                break;
            }
        }

    if (!sapphiron || !sapphiron->IsAlive())
        return false;

    plan.assignments.clear();
    plan.encounter = RAID_ENCOUNTER_SAPPHIRON;
    plan.label = "Sapphiron";

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
            // As on the generic bosses: with a player tanking her, a bot tank
            // is not allowed to take her off him, so it is not told to hold
            // her either.
            uint8 const tankDuty = plan.mainTankIsHuman ? RAID_DUTY_DAMAGE : RAID_DUTY_TANK;
            plan.assignments[member->GetGUID()] = {tankDuty, 0, sapphiron->GetGUID()};
            continue;
        }

        uint8 const duty = PlayerbotAI::IsHeal(member) ? RAID_DUTY_HEAL : RAID_DUTY_DAMAGE;
        bool const ranged = PlayerbotAI::IsRanged(member) || PlayerbotAI::IsHeal(member);
        plan.assignments[member->GetGUID()] = {duty, ranged ? ++slot : 0u, sapphiron->GetGUID()};
    }

    return true;
}

// ---------------------------------------------------------------------------
// Every other Naxxramas boss
//
// One builder, a table of per-encounter numbers. The value here is not clever
// positioning — the specialist actions above still own the dances, the
// kiting, the side splits — it is that every fight now has ONE resolved boss
// and ONE assignment list, instead of forty bots each asking their own threat
// list what is happening. That was the defect behind Thaddius, Gothik and the
// Horsemen alike, and it applies to all of these equally.
//
// ring == 0 means the encounter's own logic owns positioning entirely, so the
// plan is built for perception and observability and nothing is moved.
// ---------------------------------------------------------------------------
namespace
{
// One owner per decision. Every encounter has exactly one system that owns
// each bot's position and one that owns its target; the plan takes a column
// only where no bespoke action exists for it. This is load-bearing because of
// how the engine falls through: an action that returns false — which every
// bespoke choose-target does on the tick its target is already right, and
// every bespoke position does once in place — hands that same tick to
// whatever is queued below it. When the plan also claimed the column, the two
// systems alternated ticks: targets flapped between the boss and the assigned
// add, and bots played tug-of-war between two "correct" spots. The mess was
// worst exactly on the fights with the most bespoke logic, because each extra
// action was another false-return for the plan to fall through.
//
//   ring        > 0 only where NOTHING else moves ranged/healers. Bespoke
//               movement disqualifies: Anub (swarm kite + spread slots),
//               Patchwerk (12-15y radial back-off), Maexxna (rear flank +
//               wrap rescuers), Noth (blink/balcony), Heigan (dance), Loatheb
//               (fixed range spot + spore soakers), Gothik (sides), Gluth
//               (per-role spots), Grobbulus (injection drops, kited boss),
//               Thaddius (platforms/polarity), Kel'Thuzad (center + p2 ring).
//               Razuvious is 0 for one specific reason: the priests channel
//               Mind Control, their crystal action returns false mid-channel,
//               and a ring move issued on that tick BREAKS THE CHANNEL.
//   planTargets true only where no bespoke choose-target exists and the whole
//               raid genuinely wants the boss: Patchwerk, Thaddius. Everyone
//               else has add priority (crypt guards, worshippers for the
//               frenzy, skeletons, wave sides, zombies, spores, wraps,
//               guardians) that the plan must not fight.
struct NaxxEncounterSpec
{
    char const* name;
    char const* label;
    float ring;        // ranged/healer ring radius; 0 = bespoke owns movement
    bool meleeStack;   // melee pile onto the boss rather than flanking
    bool planTargets;  // plan may retarget bots onto the boss
};

NaxxEncounterSpec const NAXX_ENCOUNTERS[] = {
    // Spider wing
    {"anub'rekhan",           "Anub'Rekhan",     0.0f, false, false},
    {"grand widow faerlina",  "Faerlina",       20.0f, false, false},
    {"maexxna",               "Maexxna",         0.0f, false, false},
    // Plague wing
    {"noth the plaguebringer","Noth",            0.0f, false, false},
    {"heigan the unclean",    "Heigan",          0.0f, false, false},
    {"loatheb",               "Loatheb",         0.0f, false, false},
    // Military wing
    {"instructor razuvious",  "Razuvious",       0.0f, false, false},
    {"gothik the harvester",  "Gothik",          0.0f, false, false},
    // Construct wing
    {"patchwerk",             "Patchwerk",       0.0f, true,  true},
    {"grobbulus",             "Grobbulus",       0.0f, false, false},
    {"gluth",                 "Gluth",           0.0f, false, false},
    {"thaddius",              "Thaddius",        0.0f, false, true},
    // Frostwyrm
    {"kel'thuzad",            "Kel'Thuzad",      0.0f, false, false},
};
}  // namespace

bool NaxxRaidPlans::BuildGeneric(Player* bot, Group* group, RaidPlan& plan)
{
    PlayerbotAI* botAI = GET_PLAYERBOT_AI(bot);
    if (!botAI || bot->GetMapId() != NAXX_MAP_ID || !bot->IsInCombat())
        return false;

    // Resolved once for the group: threat first, then a grid sweep, so a bot
    // that has not yet struck anything still knows which fight it is in.
    Unit* boss = nullptr;
    NaxxEncounterSpec const* spec = nullptr;

    for (NaxxEncounterSpec const& candidate : NAXX_ENCOUNTERS)
    {
        boss = botAI->GetAiObjectContext()->GetValue<Unit*>("find target", candidate.name)->Get();
        if (boss && boss->IsAlive())
        {
            spec = &candidate;
            break;
        }
        boss = nullptr;
    }

    if (!boss)
        for (auto const& guid :
             botAI->GetAiObjectContext()->GetValue<GuidVector>("possible targets no los")->Get())
        {
            Unit* unit = botAI->GetUnit(guid);
            // Must already be fighting US. The sweep deliberately ignores line
            // of sight so a bot that has not struck anything still knows which
            // fight it is in, but it also reaches 100y through walls and
            // floors, and a bare in-combat check let bosses who patrol into a
            // trash brawl — Grobbulus through the wall, Patchwerk by proximity
            // assist — get declared as the encounter nobody pulled.
            if (!unit || !unit->IsAlive() || !EngagedWithGroup(unit, group))
                continue;

            for (NaxxEncounterSpec const& candidate : NAXX_ENCOUNTERS)
                if (botAI->EqualLowercaseName(unit->GetName(), candidate.name))
                {
                    boss = unit;
                    spec = &candidate;
                    break;
                }

            if (boss)
                break;
        }

    if (!boss || !spec)
        return false;

    plan.assignments.clear();
    plan.encounter = RAID_ENCOUNTER_NAXX_GENERIC;
    plan.label = spec->label;

    if (!plan.engagedMs)
        plan.engagedMs = getMSTime();

    plan.phase = 1;

    // The director has already censused the raid; this hands out assignments to
    // everything it can actually drive.
    uint32 ringSlot = 0;
    for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
    {
        Player* member = itr->GetSource();
        if (!member || !member->IsAlive())
            continue;

        // Anything with an AI takes an assignment, a player who typed "bot
        // self" included - the AI is what carries it out, and testing for a
        // real player instead dropped those characters out of every mechanic
        // the plan drives.
        if (!GET_PLAYERBOT_AI(member))
            continue;

        if (PlayerbotAI::IsTank(member))
        {
            // A player holding the boss changes what a bot tank is for. The
            // taunt guard forbids it to take the boss off him, so ordering it
            // to hold the boss anyway just parks it in the boss's face
            // building threat it is not allowed to convert - two halves of the
            // same feature issuing opposite orders. It fights as melee
            // instead, on hand for whatever the player has not got.
            uint8 const tankDuty = plan.mainTankIsHuman ? RAID_DUTY_DAMAGE : RAID_DUTY_TANK;
            plan.assignments[member->GetGUID()] = {tankDuty, 0, boss->GetGUID()};
            continue;
        }

        bool const ranged = PlayerbotAI::IsRanged(member) || PlayerbotAI::IsHeal(member);
        uint8 const duty = PlayerbotAI::IsHeal(member) ? RAID_DUTY_HEAL : RAID_DUTY_DAMAGE;

        // Camp 0 means "no assigned spot": melee, and everyone on encounters
        // whose own logic owns positioning.
        uint32 const camp = (ranged && spec->ring > 0.0f) ? ++ringSlot : 0u;
        plan.assignments[member->GetGUID()] = {duty, camp, boss->GetGUID()};
    }

    plan.ringSlots = ringSlot;

    // Heigan publishes his schedule (boss AI GetData 301/302, recorded at
    // the script's own Schedule sites). The fast-dance start is the one that
    // matters: its first eruption lands 7s after a transition bots could
    // otherwise only detect ~1.2s in, via the Plague Cloud aura — too late
    // for the far ring's ~6s walk off the platform. Publish the transition
    // through its entry window, then fall back to the eruption cadence.
    if (strcmp(spec->label, "Heigan") == 0)
        if (Creature* creature = boss->ToCreature())
            if (CreatureAI* ai = creature->AI())
            {
                uint32 const now = getMSTime();
                uint32 const fastStart = ai->GetData(NaxxHelpers::HEIGAN_DATA_FAST_DANCE_MS);
                uint32 const nextErupt = ai->GetData(NaxxHelpers::HEIGAN_DATA_NEXT_ERUPTION_MS);

                if (fastStart && now < fastStart + 7000)
                {
                    plan.nextEventKind = RAID_EVENT_HEIGAN_FAST_DANCE;
                    plan.nextEventMs = fastStart;
                }
                else if (nextErupt && now < nextErupt)
                {
                    plan.nextEventKind = RAID_EVENT_HEIGAN_ERUPTION;
                    plan.nextEventMs = nextErupt;
                }
            }

    return true;
}

float NaxxRaidPlans::GenericRing(std::string const& label)
{
    for (NaxxEncounterSpec const& spec : NAXX_ENCOUNTERS)
        if (label == spec.label)
            return spec.ring;

    return 0.0f;
}

bool NaxxRaidPlans::GenericPlanTargets(std::string const& label)
{
    for (NaxxEncounterSpec const& spec : NAXX_ENCOUNTERS)
        if (label == spec.label)
            return spec.planTargets;

    return false;
}

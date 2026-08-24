#include "RaidDirector.h"

#include "Group.h"
#include "NaxxRaidPlans.h"
#include "Playerbots.h"
#include "Timer.h"

namespace
{
// One plan per group. Keyed on the group, not on any bot, so nothing is lost
// when a bot dies or logs out and there is no leader handover to get wrong.
std::unordered_map<ObjectGuid, RaidPlan> s_plans;

// Rebuild window. Short enough that assignments react within a global
// cooldown, long enough that forty bots ticking do not each rebuild.
constexpr uint32 REBUILD_INTERVAL_MS = 500;

// Plans for groups that stopped asking are dropped after this.
constexpr uint32 STALE_AFTER_MS = 60000;

void PruneStale(uint32 now)
{
    for (auto it = s_plans.begin(); it != s_plans.end();)
        it = (now - it->second.updatedMs > STALE_AFTER_MS) ? s_plans.erase(it) : ++it;
}

// Who the raid actually has. Taken once per rebuild, for every encounter,
// because it belongs to the plan rather than to any one builder: while this
// lived inside the generic builder, the two encounters with builders of their
// own - the Horsemen and Sapphiron - left whatever the last generic fight had
// worked out sitting in the struct, and `raidplan` reported the previous
// boss's main tank on both of them.
void Census(Player* bot, Group* group, RaidPlan& plan)
{
    plan.mainTank = PlayerbotAI::GetMainTankGuid(group, bot);
    plan.mainTankIsHuman = false;
    plan.humanTanks = 0;
    plan.humanHealers = 0;
    plan.humanDamage = 0;

    for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
    {
        Player* member = itr->GetSource();
        if (!member || member->GetMap() != bot->GetMap())
            continue;

        // Anything the AI drives carries out its own assignments, a player who
        // typed "bot self" included. Only a character nobody is scripting has
        // to be planned around, so only those are counted here.
        if (GET_PLAYERBOT_AI(member))
            continue;

        // Deliberately not gated on being alive: a main tank who is taking a
        // rez is still the person this raid is built around, and counting him
        // as absent flips every bot tank back onto the boss mid-fight.
        if (member->GetGUID() == plan.mainTank)
            plan.mainTankIsHuman = true;

        if (PlayerbotAI::IsTank(member))
            ++plan.humanTanks;
        else if (PlayerbotAI::IsHeal(member))
            ++plan.humanHealers;
        else
            ++plan.humanDamage;
    }
}

void ClearCensus(RaidPlan& plan)
{
    plan.mainTank = ObjectGuid::Empty;
    plan.mainTankIsHuman = false;
    plan.humanTanks = 0;
    plan.humanHealers = 0;
    plan.humanDamage = 0;
}
}  // namespace

void RaidDirector::Tick(Player* bot)
{
    if (!bot)
        return;

    Group* group = bot->GetGroup();
    if (!group)
        return;

    uint32 const now = getMSTime();
    RaidPlan& plan = s_plans[group->GetGUID()];

    // Kill switch. Turning this off drops the raid to plain combat AI on the
    // encounters the director drives — it is a safety valve, not a
    // preference, so leave it on unless the plans are actively misbehaving.
    if (!sPlayerbotAIConfig.raidDirector)
    {
        plan.encounter = RAID_ENCOUNTER_NONE;
        plan.phase = 0;
        plan.engagedMs = 0;
        plan.label.clear();
        plan.assignments.clear();
        plan.ringSlots = 0;
        plan.nextEventKind = RAID_EVENT_NONE;
        plan.nextEventMs = 0;
        ClearCensus(plan);
        plan.updatedMs = now;
        return;
    }

    // Whoever gets here first inside the window does the work for everyone.
    if (plan.updatedMs && now - plan.updatedMs < REBUILD_INTERVAL_MS)
        return;

    plan.updatedMs = now;

    // Deadlines are per-rebuild outputs: a builder that knows the schedule
    // republishes it every pass, so anything left over here is stale.
    plan.nextEventKind = RAID_EVENT_NONE;
    plan.nextEventMs = 0;

    // Before the builders, because they read it: whether the raid already has
    // somebody holding the boss decides what the bot tanks are told to do.
    Census(bot, group, plan);

    // Encounter builders own everything below the plan: which fight this is,
    // who holds what, and when that changes. They receive the plan carrying
    // its previous assignments so decisions can persist rather than being
    // re-derived from a world the previous decision just altered.
    if (!NaxxRaidPlans::BuildFourHorsemen(bot, group, plan) &&
        !NaxxRaidPlans::BuildSapphiron(bot, group, plan) &&
        !NaxxRaidPlans::BuildGeneric(bot, group, plan))
    {
        plan.encounter = RAID_ENCOUNTER_NONE;
        plan.phase = 0;
        plan.engagedMs = 0;
        plan.label.clear();
        plan.assignments.clear();
        plan.ringSlots = 0;
        plan.nextEventKind = RAID_EVENT_NONE;
        plan.nextEventMs = 0;
        ClearCensus(plan);
    }

    PruneStale(now);
}

RaidPlan const* RaidDirector::Get(Player* bot)
{
    if (!bot)
        return nullptr;

    Group* group = bot->GetGroup();
    if (!group)
        return nullptr;

    auto it = s_plans.find(group->GetGUID());
    if (it == s_plans.end() || it->second.encounter == RAID_ENCOUNTER_NONE)
        return nullptr;

    return &it->second;
}

bool RaidDirectorTickAction::Execute(Event /*event*/)
{
    RaidDirector::Tick(bot);
    return false;  // never consumes the tick; it only keeps the plan fresh
}

bool RaidPlanAction::Execute(Event /*event*/)
{
    // The command reaches every bot in the group; only the first living one
    // answers, or the reply arrives forty times over.
    if (Group* group = bot->GetGroup())
        for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
        {
            Player* member = itr->GetSource();
            if (!member || !member->IsAlive() || !GET_PLAYERBOT_AI(member))
                continue;

            if (member != bot)
                return false;

            break;
        }

    RaidDirector::Tick(bot);
    std::string const text = RaidDirector::Describe(bot);

    // One whisper per line: the client truncates long single messages.
    std::string line;
    for (char c : text)
    {
        if (c != '\n')
        {
            line += c;
            continue;
        }

        if (!line.empty())
            botAI->TellMaster(line);

        line.clear();
    }

    if (!line.empty())
        botAI->TellMaster(line);

    return true;
}

std::string RaidDirector::Describe(Player* bot)
{
    RaidPlan const* plan = RaidDirector::Get(bot);
    if (!plan)
    {
        // "No plan" has several very different causes, and guessing between
        // them from outside the server is exactly the round trip this
        // command exists to avoid.
        if (!sPlayerbotAIConfig.raidDirector)
            return "No raid plan: the director is disabled (AiPlayerbot.RaidDirector = 0).";

        if (!bot->GetGroup())
            return "No raid plan: I am not in a group.";

        Map* map = bot->GetMap();
        if (!map || !map->IsRaid())
            return "No raid plan: this is not a raid map.";

        if (!bot->IsInCombat())
            return "No raid plan: the raid is not in combat. Plans are built on the pull.";

        // Free function: no botAI member here, unlike an Action.
        if (PlayerbotAI* botAI = GET_PLAYERBOT_AI(bot))
            if (!botAI->HasStrategy("naxx", BOT_STATE_COMBAT))
                return "No raid plan: the naxx strategy is not active on me (say 'naxx' to enable).";

        return "No raid plan: in combat on a raid map, but no encounter the director "
               "recognised here.";
    }

    static char const* dutyName[] = {"idle", "tank", "reserve", "damage", "heal", "hide"};

    uint32 counts[6] = {};
    std::string lines;

    for (auto const& [guid, assignment] : plan->assignments)
    {
        if (assignment.duty < 6)
            ++counts[assignment.duty];

        // Tanks and reserves are the interesting rows: everything that has
        // gone wrong on this fight has been visible in those two lines.
        if (assignment.duty != RAID_DUTY_TANK && assignment.duty != RAID_DUTY_RESERVE)
            continue;

        Player* member = ObjectAccessor::FindPlayer(guid);
        lines += "  " + std::string(member ? member->GetName() : "?") + ": " +
                 dutyName[assignment.duty] + " camp " + std::to_string(assignment.camp) + "\n";
    }

    std::string tankLine = "  main tank: ";
    if (Player* mt = ObjectAccessor::FindPlayer(plan->mainTank))
        tankLine += mt->GetName() + (plan->mainTankIsHuman ? " (player)" : " (bot)");
    else
        tankLine += "none";

    if (plan->humanTanks || plan->humanHealers || plan->humanDamage)
        tankLine += "\n  players in raid: " + std::to_string(plan->humanTanks) + " tank, " +
                    std::to_string(plan->humanHealers) + " heal, " +
                    std::to_string(plan->humanDamage) + " damage";

    return (plan->label.empty() ? std::string("Encounter") : plan->label) +
           ", phase " + std::to_string(plan->phase) +
           " — " + std::to_string(counts[RAID_DUTY_TANK]) + " tank, " +
           std::to_string(counts[RAID_DUTY_RESERVE]) + " reserve, " +
           std::to_string(counts[RAID_DUTY_DAMAGE]) + " damage, " +
           std::to_string(counts[RAID_DUTY_HEAL]) + " heal, " +
           std::to_string(counts[RAID_DUTY_HIDE]) + " hiding\n" + tankLine + "\n" + lines;
}

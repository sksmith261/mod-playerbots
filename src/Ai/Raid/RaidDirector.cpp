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

    // Whoever gets here first inside the window does the work for everyone.
    if (plan.updatedMs && now - plan.updatedMs < REBUILD_INTERVAL_MS)
        return;

    plan.updatedMs = now;

    // Encounter builders own everything below the plan: which fight this is,
    // who holds what, and when that changes. They receive the plan carrying
    // its previous assignments so decisions can persist rather than being
    // re-derived from a world the previous decision just altered.
    if (!NaxxRaidPlans::BuildFourHorsemen(bot, group, plan) &&
        !NaxxRaidPlans::BuildSapphiron(bot, group, plan))
    {
        plan.encounter = RAID_ENCOUNTER_NONE;
        plan.phase = 0;
        plan.engagedMs = 0;
        plan.assignments.clear();
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
        return "No raid plan is running for this group.";

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

    return "Encounter " + std::to_string(plan->encounter) + ", phase " + std::to_string(plan->phase) +
           " — " + std::to_string(counts[RAID_DUTY_TANK]) + " tank, " +
           std::to_string(counts[RAID_DUTY_RESERVE]) + " reserve, " +
           std::to_string(counts[RAID_DUTY_DAMAGE]) + " damage, " +
           std::to_string(counts[RAID_DUTY_HEAL]) + " heal, " +
           std::to_string(counts[RAID_DUTY_HIDE]) + " hiding\n" + lines;
}

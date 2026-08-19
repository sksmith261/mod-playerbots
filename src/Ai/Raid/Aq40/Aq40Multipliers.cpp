#include "Aq40Multipliers.h"

#include "Aq40Utils.h"
#include "Playerbots.h"

float Aq40TwinsDutyMultiplier::GetValue(Action* action)
{
    if (!action)
        return 1.0f;

    // Tanks and healers manage their own targets (tank pickup owns tanks;
    // heal targets are friendly and never match a twin).
    if (PlayerbotAI::IsTank(bot) || PlayerbotAI::IsHeal(bot))
        return 1.0f;

    Unit* veklor = AI_VALUE2(Unit*, "find target", "emperor vek'lor");
    Unit* veknilash = AI_VALUE2(Unit*, "find target", "emperor vek'nilash");
    if (!veklor || !veknilash || !veknilash->IsInCombat())
        return 1.0f;

    Unit* wrongTwin = RaidAq40::IsCasterDps(bot) ? veknilash : veklor;
    if (action->GetTarget() != wrongTwin)
        return 1.0f;

    return 0.0f;
}

#include "BWLTriggers.h"

#include "Playerbots.h"
#include "BWLHelpers.h"

using namespace BlackwingLairHelpers;

// General

bool BwlSuppressionDeviceTrigger::IsActive()
{
    // Until MoP, only rogues could disarm suppression devices.
    // If raid cheats are enabled, any bot can disarm the devices.
    if (botAI->HasCheat(BotCheatMask::raid) || bot->IsClass(CLASS_ROGUE))
    {
        GuidVector gos = AI_VALUE(GuidVector, "nearest game objects");
        for (auto i = gos.begin(); i != gos.end(); ++i)
        {
            const GameObject* go = botAI->GetGameObject(*i);
            if (IsActiveSuppressionDeviceInRange(go, bot))
                return true;
        }
    }
    return false;
}

// Razorgore the Untamed

bool BwlRazorgoreNotMindControlledTrigger::IsActive()
{
    if (Unit* boss = AI_VALUE2(Unit*, "find target", "razorgore the untamed"))
        return !boss->HasAura(static_cast<uint32>(BlackwingLairSpells::SPELL_MINDCONTROL));
    return false;
}

// Vaelastrasz the Corrupt

bool BwlVaelastraszPositioningTrigger::IsActive()
{
    // Prevent non-tanks from rotating the boss while the tanks gain thread.
    if (Unit* boss = AI_VALUE2(Unit*, "find target", "vaelastrasz the corrupt"))
        return boss->GetVictim() != bot;
    return false;
}

bool BwlVaelastraszBurningAdrenalineTrigger::IsActive()
{
    // No check for Vaelastrasz, because bots may still have burning adrenaline even after Vaelastrasz died.
    return bot->HasAura(static_cast<uint32>(BlackwingLairSpells::SPELL_BURNING_ADRENALINE));
}

// Chromaggus

bool BwlAfflictionBronzeTrigger::IsActive()
{
    return bot->HasAura(static_cast<uint32>(BlackwingLairSpells::SPELL_BROOD_AFFLICTION_BRONZE));
}

// Nefarian

bool BwlWildMagicTrigger::IsActive()
{
    return bot->getClass() == CLASS_MAGE &&
        bot->HasAura(static_cast<uint32>(BlackwingLairSpells::SPELL_WILD_MAGIC));
}

bool BwlNefarianFearWardTrigger::IsActive()
{
    if (bot->getClass() != CLASS_PRIEST)
        return false;

    Unit* nefarian = AI_VALUE2(Unit*, "find target", "nefarian");
    if (!nefarian || !nefarian->IsInCombat())
        return false;

    Unit* victim = nefarian->GetVictim();
    if (!victim)
        return false;

    return !botAI->HasAura("fear ward", victim);
}

// Trash

bool BwlDeathTalonWyrmguardTankTrigger::IsActive()
{
    return PlayerbotAI::IsTank(bot) && AI_VALUE2(Unit*, "find target", "death talon wyrmguard");
}

bool BwlDeathTalonWyrmguardRangedTrigger::IsActive()
{
    return PlayerbotAI::IsRanged(bot) && AI_VALUE2(Unit*, "find target", "death talon wyrmguard");
}

// A tank the election may pick: it must be bot-controlled (a human tank
// cannot be commanded to taunt) and of a class whose taunt the taunt action
// can actually execute — "taunt spell" is aliased only by warrior/paladin/DK
// tank strategies, druids use "growl" (handled in BwlEbonrocTauntAction).
static bool CanExecuteEbonrocTaunt(Player* member)
{
    if (!GET_PLAYERBOT_AI(member))
        return false;

    switch (member->getClass())
    {
        case CLASS_WARRIOR:
        case CLASS_PALADIN:
        case CLASS_DEATH_KNIGHT:
        case CLASS_DRUID:
            return true;
        default:
            return false;
    }
}

bool BwlEbonrocShadowSwapTrigger::IsActive()
{
    using namespace BlackwingLairHelpers;

    if (!PlayerbotAI::IsTank(bot))
        return false;

    Unit* boss = AI_VALUE2(Unit*, "find target", "ebonroc");
    if (!boss)
        return false;

    Unit* victim = boss->GetVictim();
    if (!victim || victim == bot)
        return false;

    uint32 const shadow = static_cast<uint32>(BlackwingLairSpells::SPELL_SHADOW_OF_EBONROC);
    if (!victim->HasAura(shadow) || bot->HasAura(shadow))
        return false;

    // Deterministic single taker: the first living tank in shared group
    // iteration order that is neither shadowed nor the current victim —
    // skipping tanks that could never execute the taunt, or the election
    // deadlocks on them and the swap never happens.
    if (Group* group = bot->GetGroup())
        for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
        {
            Player* member = itr->GetSource();
            if (!member || !member->IsAlive() || !PlayerbotAI::IsTank(member))
                continue;

            if (member == victim || member->HasAura(shadow))
                continue;

            if (!CanExecuteEbonrocTaunt(member))
                continue;

            return member == bot;
        }

    return false;
}

// Core boss_nefarian.cpp spawnerPositions.
extern Position const NEFARIAN_DOOR_CAMPS[2];
Position const NEFARIAN_DOOR_CAMPS[2] = {
    { -7599.32f, -1191.72f, 475.545f },
    { -7526.27f, -1135.04f, 473.445f },
};

int32 BlackwingLairHelpers::GetNefarianDoorAssignment(PlayerbotAI* botAI, Player* bot)
{
    // Parity split among living bot members in shared group order.
    int32 myIndex = -1;
    uint32 count = 0;
    if (Group* group = bot->GetGroup())
        for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
        {
            Player* member = itr->GetSource();
            if (!member || !member->IsAlive() || !GET_PLAYERBOT_AI(member))
                continue;

            if (member == bot)
                myIndex = count;

            ++count;
        }

    return myIndex < 0 ? -1 : myIndex % 2;
}

bool BwlNefarianDoorCampTrigger::IsActive()
{
    using namespace BlackwingLairHelpers;

    if (!bot->IsAlive() || !IsRaidGroupInCombat(bot))
        return false;

    // P1 only: drakonids streaming and Nefarian himself not yet landed.
    if (AI_VALUE2(Unit*, "find target", "nefarian"))
        return false;

    bool drakonidAlive = false;
    for (auto const& target : AI_VALUE(GuidVector, "possible targets no los"))
    {
        for (uint32 entry : NEFARIAN_DRAKONIDS)
            if (target.GetEntry() == entry)
            {
                Unit* unit = botAI->GetUnit(target);
                if (unit && unit->IsAlive())
                {
                    drakonidAlive = true;
                    break;
                }
            }

        if (drakonidAlive)
            break;
    }

    if (!drakonidAlive)
        return false;

    int32 door = GetNefarianDoorAssignment(botAI, bot);
    if (door < 0)
        return false;

    return bot->GetExactDist2d(NEFARIAN_DOOR_CAMPS[door].GetPositionX(), NEFARIAN_DOOR_CAMPS[door].GetPositionY()) > 25.0f;
}

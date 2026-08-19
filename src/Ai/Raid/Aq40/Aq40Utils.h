#ifndef PLAYERBOTS_AQ40UTILS_H
#define PLAYERBOTS_AQ40UTILS_H

#include <cmath>

#include "Define.h"
#include "PlayerbotAI.h"

namespace RaidAq40
{
    // Signed smallest angular difference a-b, in [-pi, pi].
    inline float AngleDelta(float a, float b)
    {
        float d = std::fmod(a - b, 2.0f * static_cast<float>(M_PI));
        if (d > static_cast<float>(M_PI))
            d -= 2.0f * static_cast<float>(M_PI);
        else if (d < -static_cast<float>(M_PI))
            d += 2.0f * static_cast<float>(M_PI);
        return d;
    }

    constexpr uint32 MAP_TEMPLE_OF_AHNQIRAJ = 531;

    // C'Thun stomach (see core boss_cthun.cpp / areatrigger 4033)
    constexpr uint32 SPELL_DIGESTIVE_ACID = 26476;
    constexpr uint32 NPC_FLESH_TENTACLE = 15802;
    constexpr uint32 AREATRIGGER_STOMACH_EXIT = 4033;

    // areatrigger 4033: map 531, radius 5
    constexpr float STOMACH_EXIT_X = -8546.23f;
    constexpr float STOMACH_EXIT_Y = 1987.65f;
    constexpr float STOMACH_EXIT_Z = -96.5207f;

    // Players are swallowed to { -8562, 2037, -70 } and fall to the stomach
    // floor (~ -98). C'Thun's chamber above sits at z ~ +100, so anything this
    // deep near the chamber is inside the stomach.
    constexpr float STOMACH_X = -8562.0f;
    constexpr float STOMACH_Y = 2037.0f;
    constexpr float STOMACH_MAX_Z = -30.0f;
    constexpr float STOMACH_RANGE_2D = 150.0f;

    // Leave the stomach once Digestive Acid stacks this high, even if a Flesh
    // Tentacle is still alive.
    constexpr uint32 EXIT_ACID_STACKS = 5;

    // Eye of C'Thun — Dark Glare (see core boss_cthun.cpp). During the glare
    // phase the eye freezes with SPELL_RED_COLORATION up and sweeps the beam
    // along its facing, rotating pi/35 rad per second for 35s in a random
    // direction. The red aura goes up ~2s before the first damage tick,
    // giving bots a head start; the eye's live orientation IS the beam angle.
    constexpr uint32 NPC_EYE_OF_CTHUN = 15589;
    constexpr uint32 SPELL_RED_COLORATION = 22518;

    // React when the beam is within this angle of the bot (rad). The beam
    // gains ~0.09 rad/s; a running bot at 30y gains ~0.23 rad/s tangentially,
    // so this margin covers reaction plus travel whichever way it rotates.
    constexpr float DARK_GLARE_DANGER_ARC = 0.7f;

    // C'Thun ring: every bot holds an assigned slot on a circle around the
    // eye for the whole fight. This is what keeps the raid off the entrance
    // stairs (where beams and the glare hit a clump) and caps P2 giant eye
    // tentacle chain-beam jumps. 30y keeps every caster in range; ~5y
    // spacing at 40 slots. Center is C'Thun's static spawn point.
    constexpr float CTHUN_CENTER_X = -8578.65f;
    constexpr float CTHUN_CENTER_Y = 1985.85f;
    constexpr float CTHUN_CENTER_Z = 100.304f;
    constexpr float CTHUN_RING_RADIUS = 30.0f;
    constexpr float CTHUN_RING_TOLERANCE = 4.0f;

    // Tentacles a bot may leave its ring slot to fight locally (they spawn
    // on top of players, i.e. on the ring itself).
    constexpr uint32 NPC_CLAW_TENTACLE = 15725;
    constexpr uint32 NPC_EYE_TENTACLE = 15726;
    constexpr uint32 NPC_GIANT_CLAW_TENTACLE_ = 15728;
    constexpr uint32 NPC_GIANT_EYE_TENTACLE = 15334;

    inline bool IsCthunTentacle(uint32 entry)
    {
        return entry == NPC_CLAW_TENTACLE || entry == NPC_EYE_TENTACLE ||
               entry == NPC_GIANT_CLAW_TENTACLE_ || entry == NPC_GIANT_EYE_TENTACLE ||
               entry == NPC_FLESH_TENTACLE;
    }

    // False when the bot has no slot (not in a group / not a bot).
    bool GetCthunRingSlot(PlayerbotAI* botAI, Player* bot, float& x, float& y, float& z);

    // The Prophet Skeram: images are TempSummons of the boss entry; the
    // real one is the only non-summon.
    constexpr uint32 NPC_PROPHET_SKERAM = 15263;

    // Battleguard Sartura: run from anything whirlwinding nearby.
    constexpr uint32 SPELL_SARTURA_WHIRLWIND = 26083;
    constexpr uint32 SPELL_GUARD_WHIRLWIND = 26038;
    constexpr uint32 NPC_SARTURA_ROYAL_GUARD = 15984;
    constexpr float WHIRLWIND_DANGER_RANGE = 10.0f;
    constexpr float WHIRLWIND_FLEE_DISTANCE = 12.0f;

    // Bug Trio kill order: Yauj (heals) -> Kri -> Vem last (his death buffs
    // the survivors with Vengeance).
    constexpr uint32 NPC_PRINCESS_YAUJ = 15543;
    constexpr uint32 NPC_LORD_KRI = 15511;
    constexpr uint32 NPC_VEM = 15544;

    // Fankriss: enraging worm adds die first.
    constexpr uint32 NPC_SPAWN_OF_FANKRISS = 15630;

    // Princess Huhuran frenzy (hunter tranq).
    constexpr uint32 SPELL_HUHURAN_FRENZY = 26051;

    // C'Thun P2: a Giant Claw Tentacle with no player within 5y full-heals
    // and resubmerges — someone must sit on it.
    constexpr uint32 NPC_GIANT_CLAW_TENTACLE = 15728;
    constexpr float GIANT_CLAW_SIT_RANGE = 4.0f;

    // Skeram pickup assignment (defined in Aq40Triggers.cpp; shared with
    // the action so both compute identical assignments).
    Unit* GetSkeramPickupAssignment(PlayerbotAI* botAI, Player* bot);
    Player* GetSkeramHealerTank(PlayerbotAI* botAI, Player* bot);

    // Zero-code positioning registrations (audit batch 1)
    constexpr uint32 SPELL_KRI_POISON_CLOUD = 26590;
    constexpr uint32 SPELL_VISCIDUS_TOXIN = 26575;
    constexpr uint32 SPELL_HUHURAN_NOXIOUS_POISON = 26053;
    constexpr uint32 SPELL_VEKLOR_BLIZZARD = 26607;
    constexpr uint32 NPC_OURO_DIRT_MOUND = 15712;
    constexpr uint32 NPC_VEKNISS_HATCHLING = 15962;
    constexpr float OURO_MOUND_FLEE_RANGE = 20.0f;

    // Twin Emperors positioning (core boss_twinemperors.cpp): they heal
    // each other within 60y; Vek'lor teleports to his victim beyond 45y and
    // Arcane Bursts point-blank.
    // Fixed camps at the twins' spawn thrones (155y apart, from creature
    // spawns): each duty team parks at its twin's camp. On teleport swaps
    // the teams cross; Vek'lor teleports to his victim beyond 45y, so a
    // victim standing at the caster camp snaps him back automatically.
    constexpr float TWINS_CAMP_VEKNILASH_X = -9023.67f;
    constexpr float TWINS_CAMP_VEKNILASH_Y = 1176.24f;
    constexpr float TWINS_CAMP_VEKNILASH_Z = -104.23f;
    constexpr float TWINS_CAMP_VEKLOR_X = -8868.31f;
    constexpr float TWINS_CAMP_VEKLOR_Y = 1205.97f;
    constexpr float TWINS_CAMP_VEKLOR_Z = -104.23f;

    constexpr float TWINS_SEPARATION_RANGE = 70.0f;
    constexpr float TWINS_SEPARATION_STEP = 30.0f;

    // The whole caster team keeps this far from Vek'nilash: Vek'lor walks to
    // caster range of his threat targets, so a clumped caster camp parks him
    // inside the 60y mutual-heal bubble no matter what his victim does.
    constexpr float TWINS_TEAM_SPACING = 35.0f;
    constexpr float TWINS_CASTER_MIN_RANGE = 12.0f;
    constexpr float TWINS_CASTER_FLEE_DISTANCE = 18.0f;

    // Twin Emperors duty split: spell damage hurts Vek'lor, physical damage
    // hurts Vek'nilash. Hunters are ranged PHYSICAL, so they share
    // Vek'nilash duty with the melee.
    inline bool IsCasterDps(Player* bot)
    {
        if (!PlayerbotAI::IsRanged(bot) || PlayerbotAI::IsHeal(bot))
            return false;

        switch (bot->getClass())
        {
            case CLASS_MAGE:
            case CLASS_WARLOCK:
            case CLASS_PRIEST:
            case CLASS_DRUID:
            case CLASS_SHAMAN:
                return true;
            default:
                return false;
        }
    }

    // How far past the current angular gap to run per dodge step (rad).
    constexpr float DARK_GLARE_DODGE_STEP = 0.5f;

    // Engagement ring while dodging: keep the bot's current distance to the
    // eye, clamped into this band.
    constexpr float DARK_GLARE_MIN_RANGE = 10.0f;
    constexpr float DARK_GLARE_MAX_RANGE = 40.0f;
}

#endif

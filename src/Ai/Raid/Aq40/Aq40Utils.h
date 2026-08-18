#ifndef PLAYERBOTS_AQ40UTILS_H
#define PLAYERBOTS_AQ40UTILS_H

#include "Define.h"

namespace RaidAq40
{
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
}

#endif

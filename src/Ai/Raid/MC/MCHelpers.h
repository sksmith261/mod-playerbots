#ifndef PLAYERBOTS_MCHELPERS_H
#define PLAYERBOTS_MCHELPERS_H

namespace MoltenCoreHelpers
{
enum MoltenCoreNPCs
{
    // Lucifron
    NPC_FLAMEWAKER_PROTECTOR = 12119,

    // Magmadar (any shaman's Tremor Totem)
    NPC_TREMOR_TOTEM = 5913,

    // Golemagg
    NPC_CORE_RAGER = 11672,

    // Core Hound (trash)
    NPC_CORE_HOUND = 11671,
};
enum MoltenCoreSpells
{
    // Magmadar
    SPELL_MAGMADAR_FRENZY = 19451,
    SPELL_LAVA_BOMB_DOT = 19428,      // periodic fire while standing in the bomb's patch (GO 177704)
    SPELL_TREMOR_TOTEM = 8143,
    SPELL_FEAR_WARD = 6346,

    // Baron Geddon
    SPELL_INFERNO = 19695,
    SPELL_LIVING_BOMB = 20475,

    // Golemagg
    SPELL_GOLEMAGGS_TRUST = 20553,
};
}

#endif

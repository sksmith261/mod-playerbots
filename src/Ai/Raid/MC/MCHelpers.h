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

    // Gehennas
    NPC_FLAMEWAKER = 11661,

    // Garr
    NPC_FIRESWORN = 12099,

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

    // Garr
    SPELL_BANISH_R1 = 710,
    SPELL_BANISH_R2 = 18647,
    SPELL_SEPARATION_ANXIETY_MINION = 23492,  // banish immunity + 300% damage when dragged from Garr

    // Gehennas
    SPELL_GEHENNAS_CURSE = 19716,  // -75% healing received; decurse target
    SPELL_RAIN_OF_FIRE = 19717,

    // Majordomo Executus (shields rotated onto his adds)
    SPELL_DOMO_MAGIC_REFLECTION = 20619,
    SPELL_DOMO_DAMAGE_REFLECTION = 21075,

    // Shazzrah
    SPELL_DEADEN_MAGIC = 19714,  // -50% magic taken self-buff; purgeable
    SPELL_PURGE_R1 = 370,
    SPELL_DISPEL_MAGIC_R1 = 527,

    // Baron Geddon
    SPELL_INFERNO = 19695,
    SPELL_LIVING_BOMB = 20475,

    // Golemagg
    SPELL_GOLEMAGGS_TRUST = 20553,
};
}

#endif

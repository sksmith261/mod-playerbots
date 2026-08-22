-- Reizan tuning: Gluth's Zombie Chows softened, not removed — the wave
-- pacing, Decimate crawl, and devour-heal all stay. Keyed on name because
-- mod-individual-progression clones the naxx40 creatures to offset entries.
-- NEVER edit this file after it has been applied - the updater tracks it by
-- hash and a re-apply would compound the relative UPDATEs. New file only.
UPDATE `creature_template` SET `HealthModifier` = `HealthModifier` * 0.6,
    `DamageModifier` = `DamageModifier` * 0.7 WHERE `name` = 'Zombie Chow';

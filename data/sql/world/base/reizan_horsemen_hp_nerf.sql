-- Reizan tuning: the Four Horsemen (naxx-40 clones) at 75% health, so the
-- fight resolves in fewer Mark rotations. Companion to the 75% damage pass
-- in reizan_horsemen_damage_nerf.sql.
--   Mograine 160 -> 120, Zeliek 150 -> 112.5,
--   Korth'azz 180 -> 135, Blaumeux 150 -> 112.5
-- NEVER edit this file after it has been applied - the updater tracks it by
-- hash and a re-apply would compound the relative UPDATE. New file only.
UPDATE `creature_template` SET `HealthModifier` = `HealthModifier` * 0.75
WHERE `entry` IN (351037, 351038, 351039, 351040);

-- Reizan tuning, same pattern as reizan_twins_hp_nerf.sql: Flesh Tentacles
-- (C'Thun stomach, entry 15802) at 25% health so the swallowed group can
-- open Weakened windows at a bot-raid pace.
-- NEVER edit this file after it has been applied - the updater tracks it by
-- hash and a re-apply would compound the relative UPDATE. New file only.
UPDATE `creature_template` SET `HealthModifier` = `HealthModifier` * 0.25 WHERE `entry` = 15802;

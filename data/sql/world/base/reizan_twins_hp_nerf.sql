-- Reizan tuning: Twin Emperors at 25% of their previous max health.
-- Relative multiply so it stacks correctly on whatever base/module value is
-- in effect. Twin Empathy shares damage between them, so both are reduced
-- equally.
-- !! Applied ONCE by the hash-tracked module updater. NEVER edit this file
-- (a re-apply would multiply again); to retune, add a NEW file setting an
-- absolute value.
UPDATE `creature_template` SET `HealthModifier` = `HealthModifier` * 0.25 WHERE `entry` IN (15275, 15276);

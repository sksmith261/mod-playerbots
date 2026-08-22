-- Reizan tuning: the Four Horsemen (naxx-40 clones) hit for 75% of their
-- previous damage. Melee only — the Marks, Meteor, Void Zone and Holy
-- Wrath are spell damage and are untouched, so the fight's mechanics still
-- punish standing in the wrong place exactly as before.
-- NEVER edit this file after it has been applied - the updater tracks it by
-- hash and a re-apply would compound the relative UPDATE. New file only.
UPDATE `creature_template` SET `DamageModifier` = `DamageModifier` * 0.75
WHERE `entry` IN (351037, 351038, 351039, 351040);

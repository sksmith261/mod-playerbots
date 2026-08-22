-- Correction to reizan_naxx_boss_teleports.sql, which has already been
-- applied — the updater tracks that file by hash, so this is a new file
-- rather than an edit to it.
--
-- NaxxHorsemen (61011) moves to the spot captured in-game with .tele add:
-- the chamber approach roughly 90y back from the horsemen, facing them.
-- The derived original sat 39y out on the far side of the room.
--
-- NEVER edit this file after it has been applied. Corrections go in a NEW file.
UPDATE `game_tele` SET `position_x` = 2582.97, `position_y` = -3012.55,
    `position_z` = 241.47, `orientation` = 2.3
WHERE `id` = 61011;

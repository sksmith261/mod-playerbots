-- Reizan testing convenience: one teleport per Naxxramas boss, landing at a
-- safe standing spot near each boss room so encounters can be retested
-- without re-clearing a wing.
--   .tele name <character> NaxxThaddius
--
-- Coordinates are not hand-guessed: each is an existing walkable point taken
-- from the live world DB - either a trash spawn point or a room door - at
-- least ~27y from the boss (outside a level-63 aggro radius) and facing it.
-- NaxxKelthuzad is the one exception: his chamber and its approach contain
-- no spawns or doors at all (every add is summoned), so that entry uses the
-- playerbot module's own room-centre with Kel'Thuzad's floor height.
--
-- NaxxThaddius was captured in-game with .tele add at the room entrance,
-- equidistant from both pet platforms and facing in.
--
-- NEVER edit this file after it has been applied - the updater tracks it by
-- hash. Corrections go in a NEW file.
DELETE FROM `game_tele` WHERE `id` BETWEEN 61003 AND 61017;
INSERT INTO `game_tele` (`id`, `position_x`, `position_y`, `position_z`, `orientation`, `map`, `name`) VALUES
(61003, 3287.81, -3450.87, 287.08, 5.56, 533, 'NaxxAnub'),  -- 38y from the boss
(61004, 3335.37, -3667.11, 259.08, 1.21, 533, 'NaxxFaerlina'),  -- 50y from the boss
(61005, 3465.16, -3940.45, 308.79, 0.39, 533, 'NaxxMaexxna'),  -- 50y from the boss
(61006, 2635.35, -3522.12, 261.93, 0.66, 533, 'NaxxNoth'),  -- 51y from the boss
(61007, 2789.62, -3752.53, 274.98, 1.48, 533, 'NaxxHeigan'),  -- 45y from the boss
(61008, 2909.00, -4025.02, 273.48, 1.57, 533, 'NaxxLoatheb'),  -- 28y from the boss
(61009, 2798.35, -3110.24, 267.77, 2.86, 533, 'NaxxRazuvious'),  -- 44y from the boss
(61010, 2643.73, -3321.73, 284.23, 4.69, 533, 'NaxxGothik'),  -- 65y from the boss
(61011, 2493.02, -2921.78, 241.19, 5.65, 533, 'NaxxHorsemen'),  -- 39y from the boss
(61012, 3279.03, -3262.90, 292.68, 0.81, 533, 'NaxxPatchwerk'),  -- 43y from the boss
(61013, 3178.61, -3263.67, 316.43, 5.04, 533, 'NaxxGrobbulus'),  -- 83y from the boss
(61014, 3339.16, -3100.64, 296.81, 3.93, 533, 'NaxxGluth'),  -- 79y from the boss
(61015, 3431.45, -3008.65, 295.61, 0.81, 533, 'NaxxThaddius'),  -- room entrance, ~79y from either pet
(61016, 3536.81, -5158.41, 142.86, 4.53, 533, 'NaxxSapphiron'),  -- 80y from the boss
(61017, 3716.19, -5106.58, 142.03, 6.06, 533, 'NaxxKelthuzad');  -- 31y from the boss

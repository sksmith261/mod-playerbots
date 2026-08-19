-- Reizan testing convenience: teleport points deep inside raids so repeated
-- boss testing doesn't require reclearing approach gauntlets after resets.
-- Usage: .tele name <character> TwinsRoom
-- Coordinates taken from verified trash spawn points (walkable, out of boss
-- aggro range; twins stand ~90y further south).
DELETE FROM `game_tele` WHERE `id` = 61001;
INSERT INTO `game_tele` (`id`, `position_x`, `position_y`, `position_z`, `orientation`, `map`, `name`) VALUES
(61001, -8916.75, 1288.38, -112.294, 4.6, 531, 'TwinsRoom');

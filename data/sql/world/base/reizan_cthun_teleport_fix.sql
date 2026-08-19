-- Fix for reizan_cthun_teleport.sql (applied, so a new file): the first
-- CthunRoom landed at the brood-NPC alcove near the instance entrance,
-- not at C'Thun. New spot: the chamber approach at the hallway mouth,
-- z from a verified creature spawn on that floor, ~94y from C'Thun's
-- spawn point — just outside his 90y aggro scan, facing him.
DELETE FROM `game_tele` WHERE `id` = 61002;
INSERT INTO `game_tele` (`id`, `position_x`, `position_y`, `position_z`, `orientation`, `map`, `name`) VALUES
(61002, -8629.5, 1906.0, 108.6, 1.0, 531, 'CthunRoom');

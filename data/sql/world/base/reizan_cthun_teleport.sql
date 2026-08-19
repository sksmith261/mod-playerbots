-- Reizan testing convenience, same pattern as reizan_raid_teleports.sql
-- (that file is applied and must not be edited — hence a new file).
-- Usage: .tele name <character> CthunRoom
-- Lands in the safe alcove beside the friendly brood NPCs (Andorgos,
-- Kandrostrasz, Vethsera) overlooking C'Thun's chamber — z from their
-- verified spawn points, well outside C'Thun's 90y aggro scan.
DELETE FROM `game_tele` WHERE `id` = 61002;
INSERT INTO `game_tele` (`id`, `position_x`, `position_y`, `position_z`, `orientation`, `map`, `name`) VALUES
(61002, -8494.5, 1934.0, 135.75, 2.57, 531, 'CthunRoom');

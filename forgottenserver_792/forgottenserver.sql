DROP TRIGGER IF EXISTS `oncreate_players`;
DROP TRIGGER IF EXISTS `oncreate_guilds`;
DROP TRIGGER IF EXISTS `ondelete_players`;
DROP TRIGGER IF EXISTS `ondelete_guilds`;
DROP TRIGGER IF EXISTS `ondelete_accounts`;

DROP TABLE IF EXISTS `player_depotitems`;
DROP TABLE IF EXISTS `tile_items`;
DROP TABLE IF EXISTS `tiles`;
DROP TABLE IF EXISTS `bans`;
DROP TABLE IF EXISTS `house_lists`;
DROP TABLE IF EXISTS `houses`;
DROP TABLE IF EXISTS `player_items`;
DROP TABLE IF EXISTS `player_skills`;
DROP TABLE IF EXISTS `player_storage`;
DROP TABLE IF EXISTS `player_viplist`;
DROP TABLE IF EXISTS `player_spells`;
DROP TABLE IF EXISTS `player_deaths`;
DROP TABLE IF EXISTS `guild_ranks`;
DROP TABLE IF EXISTS `guilds`;
DROP TABLE IF EXISTS `guild_invites`;
DROP TABLE IF EXISTS `global_storage`;
DROP TABLE IF EXISTS `players`;
DROP TABLE IF EXISTS `accounts`;

CREATE TABLE `server_config`
(
	`config` varchar(50) NOT NULL,
	`value` varchar(256) NOT NULL DEFAULT '',
	UNIQUE KEY `config` (`config`)
) ENGINE=InnoDB;

INSERT INTO `server_config` VALUES ('db_version','2'),('encryption','0');

CREATE TABLE `accounts`
(
	`id` INT NOT NULL AUTO_INCREMENT,
	`password` VARCHAR(255) /* VARCHAR(32) for MD5*/ NOT NULL DEFAULT '',
	`type` INT NOT NULL DEFAULT 1,
	`premdays` INT NOT NULL DEFAULT 0,
	`lastday` INT UNSIGNED NOT NULL DEFAULT 0,
	`key` VARCHAR(20) NOT NULL DEFAULT '0',
	`email` VARCHAR(255) NOT NULL DEFAULT '',
	`blocked` TINYINT(1) NOT NULL DEFAULT FALSE,
	`warnings` INT NOT NULL DEFAULT 0,
	PRIMARY KEY (`id`)
) ENGINE = InnoDB;

INSERT INTO `accounts` VALUES (1, '1', 1, 65535, 0, '0', '', 0, 0);
INSERT INTO `accounts` VALUES (222222, 'tibia', 5, 65535, 0, '0', '', 0, 0);

CREATE TABLE `players`
(
	`id` INT NOT NULL AUTO_INCREMENT,
	`name` VARCHAR(255) NOT NULL,
	`group_id` INT NOT NULL DEFAULT 1,
	`account_id` INT NOT NULL DEFAULT 0,
	`level` INT NOT NULL DEFAULT 1,
	`vocation` INT NOT NULL DEFAULT 0,
	`health` INT NOT NULL DEFAULT 150,
	`healthmax` INT NOT NULL DEFAULT 150,
	`experience` BIGINT NOT NULL DEFAULT 0,
	`lookbody` INT NOT NULL DEFAULT 0,
	`lookfeet` INT NOT NULL DEFAULT 0,
	`lookhead` INT NOT NULL DEFAULT 0,
	`looklegs` INT NOT NULL DEFAULT 0,
	`looktype` INT NOT NULL DEFAULT 136,
	`lookaddons` INT NOT NULL DEFAULT 0,
	`maglevel` INT NOT NULL DEFAULT 0,
	`mana` INT NOT NULL DEFAULT 0,
	`manamax` INT NOT NULL DEFAULT 0,
	`manaspent` INT NOT NULL DEFAULT 0,
	`soul` INT UNSIGNED NOT NULL DEFAULT 0,
	`town_id` INT NOT NULL DEFAULT 0,
	`posx` INT NOT NULL DEFAULT 0,
	`posy` INT NOT NULL DEFAULT 0,
	`posz` INT NOT NULL DEFAULT 0,
	`conditions` BLOB NOT NULL,
	`cap` INT NOT NULL DEFAULT 0,
	`sex` INT NOT NULL DEFAULT 0,
	`lastlogin` BIGINT UNSIGNED NOT NULL DEFAULT 0,
	`lastip` INT UNSIGNED NOT NULL DEFAULT 0,
	`save` TINYINT(1) NOT NULL DEFAULT 1,
	`redskull` TINYINT(1) NOT NULL DEFAULT 0,
	`redskulltime` INT NOT NULL DEFAULT 0,
	`rank_id` INT NOT NULL DEFAULT 0,
	`guildnick` VARCHAR(255) NOT NULL DEFAULT '',
	`lastlogout` BIGINT UNSIGNED NOT NULL DEFAULT 0,
	`blessings` TINYINT(2) NOT NULL DEFAULT 0,
	`direction` INT NOT NULL DEFAULT 0 COMMENT 'NOT IN USE BY THE SERVER',
	`loss_experience` INT NOT NULL DEFAULT '10' COMMENT 'NOT IN USE BY THE SERVER',
	`loss_mana` INT NOT NULL DEFAULT '10' COMMENT 'NOT IN USE BY THE SERVER',
	`loss_skills` INT NOT NULL DEFAULT '10' COMMENT 'NOT IN USE BY THE SERVER',
	`premend` INT NOT NULL DEFAULT 0 COMMENT 'NOT IN USE BY THE SERVER',
	`online` TINYINT NOT NULL DEFAULT 0,
	`balance` bigint(20) unsigned NOT NULL DEFAULT '0',
	PRIMARY KEY (`id`),
	KEY (`name`),
	FOREIGN KEY (`account_id`) REFERENCES `accounts`(`id`) ON DELETE CASCADE
) ENGINE = InnoDB;

INSERT INTO `players` VALUES (1, 'Account Manager', 1, 1, 1, 0, 150, 150, 0, 0, 0, 0, 0, 9, 0, 0, 0, 0, 0, 0, 1, 1000, 1000, 7, '', 400, 0, 0, 0, 0, 0, 0, 0, '', 0, 0, 0, 10, 10, 10, 0, 0, 0);
INSERT INTO `players` VALUES (2, 'GM Tryller', 5, 222222, 1, 0, 150, 150, 0, 0, 0, 0, 0, 9, 0, 0, 0, 0, 0, 0, 1, 1000, 1000, 7, '', 400, 0, 0, 0, 1, 0, 0, 0, '', 0, 0, 0, 10, 10, 10, 0, 0, 0);

CREATE TABLE `bans`
(
	`type` INT NOT NULL COMMENT 'this field defines if its ip, accountban or namelock',
	`ip` INT UNSIGNED NOT NULL DEFAULT 0,
	`mask` INT UNSIGNED NOT NULL DEFAULT 4294967295,
	`player` INT UNSIGNED NOT NULL DEFAULT 0,
	`account` INT UNSIGNED NOT NULL DEFAULT 0,
	`time` INT UNSIGNED NOT NULL DEFAULT 0,
	`reason_id` INT NOT NULL DEFAULT 0,
	`action_id` INT NOT NULL DEFAULT 0,
	`comment` VARCHAR(60) NOT NULL DEFAULT '',
	`banned_by` INT UNSIGNED NOT NULL DEFAULT 0
) ENGINE = InnoDB;

CREATE TABLE `global_storage`
(
	`key` INT UNSIGNED NOT NULL,
	`value` INT NOT NULL,
	PRIMARY KEY  (`key`)
) ENGINE = InnoDB;

CREATE TABLE `guilds`
(
	`id` INT NOT NULL AUTO_INCREMENT,
	`name` VARCHAR(255) NOT NULL COMMENT 'guild name - nothing else needed here',
	`ownerid` INT NOT NULL,
	`creationdata` INT NOT NULL,
	`motd` VARCHAR(255) NOT NULL,
	PRIMARY KEY (`id`)
) ENGINE = InnoDB;

CREATE TABLE `guild_invites`
(
	`player_id` INT UNSIGNED NOT NULL DEFAULT 0,
	`guild_id` INT UNSIGNED NOT NULL DEFAULT 0
) ENGINE = InnoDB;

CREATE TABLE `guild_ranks`
(
	`id` INT NOT NULL AUTO_INCREMENT,
	`guild_id` INT NOT NULL COMMENT 'guild',
	`name` VARCHAR(255) NOT NULL COMMENT 'rank name',
	`level` INT NOT NULL COMMENT 'rank level - leader, vice, member, maybe something else',
	PRIMARY KEY (`id`),
	FOREIGN KEY (`guild_id`) REFERENCES `guilds`(`id`) ON DELETE CASCADE
) ENGINE = InnoDB;

CREATE TABLE `house_lists`
(
	`house_id` INT NOT NULL,
	`listid` INT NOT NULL,
	`list` TEXT NOT NULL
) ENGINE = InnoDB;

CREATE TABLE `houses`
(
	`id` INT NOT NULL AUTO_INCREMENT,
	`owner` INT NOT NULL,
	`paid` INT UNSIGNED NOT NULL DEFAULT 0,
	`warnings` TEXT NOT NULL,
	PRIMARY KEY (`id`)
) ENGINE = InnoDB;

CREATE TABLE `player_deaths`
(
	`player_id` INT NOT NULL,
	`time` BIGINT UNSIGNED NOT NULL DEFAULT 0,
	`level` INT NOT NULL DEFAULT 1,
	`killed_by` VARCHAR(255) NOT NULL,
	`is_player` TINYINT(1) NOT NULL DEFAULT 1,
	FOREIGN KEY (`player_id`) REFERENCES `players`(`id`) ON DELETE CASCADE
) ENGINE = InnoDB;

CREATE TABLE `player_depotitems`
(
	`player_id` INT NOT NULL,
	`depot_id` INT NOT NULL DEFAULT 0,
	`sid` INT NOT NULL COMMENT 'any given range eg 0-100 will be reserved for depot lockers and all > 100 will be then normal items inside depots',
	`pid` INT NOT NULL DEFAULT 0,
	`itemtype` INT NOT NULL,
	`count` INT NOT NULL DEFAULT 0,
	`attributes` BLOB NOT NULL,
	FOREIGN KEY (`player_id`) REFERENCES `players`(`id`) ON DELETE CASCADE,
	KEY (`player_id`, `depot_id`),
	UNIQUE KEY (`player_id`, `sid`)
) ENGINE = InnoDB;

CREATE TABLE `player_items`
(
	`player_id` INT NOT NULL DEFAULT 0,
	`pid` INT NOT NULL DEFAULT 0,
	`sid` INT NOT NULL DEFAULT 0,
	`itemtype` INT NOT NULL DEFAULT 0,
	`count` INT NOT NULL DEFAULT 0,
	`attributes` text NOT NULL,
	FOREIGN KEY (`player_id`) REFERENCES `players`(`id`) ON DELETE CASCADE
) ENGINE = InnoDB;

CREATE TABLE `player_skills`
(
	`player_id` INT NOT NULL DEFAULT 0,
	`skillid` tinyint(4) NOT NULL DEFAULT 0,
	`value` INT UNSIGNED NOT NULL DEFAULT 0,
	`count` INT UNSIGNED NOT NULL DEFAULT 0,
	FOREIGN KEY (`player_id`) REFERENCES `players`(`id`) ON DELETE CASCADE
) ENGINE = InnoDB;

CREATE TABLE `player_spells`
(
	`player_id` INT NOT NULL,
	`name` VARCHAR(255) NOT NULL,
	 FOREIGN KEY (`player_id`) REFERENCES `players`(`id`) ON DELETE CASCADE
) ENGINE = InnoDB;

CREATE TABLE `player_storage`
(
	`player_id` INT NOT NULL DEFAULT 0,
	`key` INT UNSIGNED NOT NULL DEFAULT 0,
	`value` INT NOT NULL DEFAULT 0,
	FOREIGN KEY (`player_id`) REFERENCES `players`(`id`) ON DELETE CASCADE
) ENGINE = InnoDB;

CREATE TABLE `player_viplist`
(
	`player_id` INT NOT NULL COMMENT 'id of player whose viplist entry it is',
	`vip_id` INT NOT NULL COMMENT 'id of target player of viplist entry',
	FOREIGN KEY (`player_id`) REFERENCES `players`(`id`) ON DELETE CASCADE,
	FOREIGN KEY (`vip_id`) REFERENCES `players`(`id`) ON DELETE CASCADE
) ENGINE = InnoDB;

CREATE TABLE `tiles`
(
	`id` INT NOT NULL AUTO_INCREMENT,
	`x` INT NOT NULL,
	`y` INT NOT NULL,
	`z` INT NOT NULL,
	PRIMARY KEY(`id`)
) ENGINE = InnoDB;

CREATE TABLE `tile_items`
(
	`tile_id` INT NOT NULL,
	`sid` INT NOT NULL,
	`pid` INT NOT NULL DEFAULT 0,
	`itemtype` INT NOT NULL,
	`count` INT NOT NULL DEFAULT 0,
	`attributes` BLOB NOT NULL,
	FOREIGN KEY (`tile_id`) REFERENCES `tiles`(`id`) ON DELETE CASCADE
) ENGINE = InnoDB;

DELIMITER |

CREATE TRIGGER `ondelete_accounts`
BEFORE DELETE
ON `accounts`
FOR EACH ROW
BEGIN
    DELETE FROM `bans` WHERE `account` = OLD.`id`;
END|

CREATE TRIGGER `ondelete_guilds`
BEFORE DELETE
ON `guilds`
FOR EACH ROW
BEGIN
    UPDATE `players` SET `guildnick` = '', `rank_id` = 0 WHERE `rank_id` IN (SELECT `id` FROM `guild_ranks` WHERE `guild_id` = OLD.`id`);
END|

CREATE TRIGGER `ondelete_players`
BEFORE DELETE
ON `players`
FOR EACH ROW
BEGIN
    DELETE FROM `bans` WHERE `type` = 2 AND `player` = OLD.`id`;
    UPDATE `houses` SET `owner` = 0 WHERE `owner` = OLD.`id`;
END|

CREATE TRIGGER `oncreate_guilds`
AFTER INSERT
ON `guilds`
FOR EACH ROW
BEGIN
    INSERT INTO `guild_ranks` (`name`, `level`, `guild_id`) VALUES ('the Leader', 3, NEW.`id`);
    INSERT INTO `guild_ranks` (`name`, `level`, `guild_id`) VALUES ('a Vice-Leader', 2, NEW.`id`);
    INSERT INTO `guild_ranks` (`name`, `level`, `guild_id`) VALUES ('a Member', 1, NEW.`id`);
END|

CREATE TRIGGER `oncreate_players`
AFTER INSERT
ON `players`
FOR EACH ROW
BEGIN
    INSERT INTO `player_skills` (`player_id`, `skillid`, `value`) VALUES (NEW.`id`, 0, 10);
    INSERT INTO `player_skills` (`player_id`, `skillid`, `value`) VALUES (NEW.`id`, 1, 10);
    INSERT INTO `player_skills` (`player_id`, `skillid`, `value`) VALUES (NEW.`id`, 2, 10);
    INSERT INTO `player_skills` (`player_id`, `skillid`, `value`) VALUES (NEW.`id`, 3, 10);
    INSERT INTO `player_skills` (`player_id`, `skillid`, `value`) VALUES (NEW.`id`, 4, 10);
    INSERT INTO `player_skills` (`player_id`, `skillid`, `value`) VALUES (NEW.`id`, 5, 10);
    INSERT INTO `player_skills` (`player_id`, `skillid`, `value`) VALUES (NEW.`id`, 6, 10);
END|

DELIMITER ;

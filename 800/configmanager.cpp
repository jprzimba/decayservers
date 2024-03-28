////////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
////////////////////////////////////////////////////////////////////////
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
////////////////////////////////////////////////////////////////////////
#include "otpch.h"
#include <iostream>

#include "configmanager.h"
#include "tools.h"

ConfigManager::ConfigManager()
{
	L = NULL;
	m_loaded = false;
	m_startup = true;

	m_confString[CONFIG_FILE] = getFilePath(FILE_TYPE_CONFIG, "config.lua");

	m_confNumber[LOGIN_PORT] = 0;
	m_confString[DATA_DIRECTORY] = m_confString[LOGS_DIRECTORY] = m_confString[IP] = m_confString[RUNFILE] = m_confString[ERROR_LOG] = m_confString[OUTPUT_LOG] = "";
	m_confBool[DAEMONIZE] = false;
}

bool ConfigManager::load()
{
	if(L)
		lua_close(L);

	L = lua_open();
	if(!L)
		return false;

	if(luaL_dofile(L, m_confString[CONFIG_FILE].c_str()))
	{
		lua_close(L);
		L = NULL;
		return false;
	}

	//parse config
	if(!m_loaded) //info that must be loaded one time (unless we reset the modules involved)
	{
		if(m_confString[DATA_DIRECTORY] == "")
			m_confString[DATA_DIRECTORY] = getGlobalString("dataDirectory", "data/");

		if(m_confString[LOGS_DIRECTORY] == "")
			m_confString[LOGS_DIRECTORY] = getGlobalString("logsDirectory", "logs/");

		if(m_confString[IP] == "")
			m_confString[IP] = getGlobalString("ip", "127.0.0.1");

		if(m_confString[RUNFILE] == "")
			m_confString[RUNFILE] = getGlobalString("runFile", "");

		if(m_confString[OUTPUT_LOG] == "")
			m_confString[OUTPUT_LOG] = getGlobalString("outputLog", "");

		if(m_confString[ERROR_LOG] == "")
			m_confString[ERROR_LOG] = getGlobalString("errorLogName", "");

		#ifdef MULTI_SQL_DRIVERS
		m_confString[SQL_TYPE] = getGlobalString("sqlType", "sqlite");
		#endif
		m_confString[SQL_HOST] = getGlobalString("sqlHost", "localhost");
		m_confString[SQL_DB] = getGlobalString("sqlDatabase", "theforgottenserver");
		m_confString[SQL_USER] = getGlobalString("sqlUser", "root");
		m_confString[SQL_PASS] = getGlobalString("sqlPass", "");
		m_confString[SQL_FILE] = getGlobalString("sqlFile", "forgottenserver.s3db");
		m_confString[MAP_NAME] = getGlobalString("mapName", "forgotten");
		m_confString[HOUSE_RENT_PERIOD] = getGlobalString("houseRentPeriod", "monthly");
		m_confString[DEFAULT_PRIORITY] = getGlobalString("defaultPriority", "high");
		m_confString[PASSWORD_TYPE] = getGlobalString("encryptionType", "plain");


		if(m_confNumber[LOGIN_PORT] == 0)
			m_confNumber[LOGIN_PORT] = getGlobalNumber("loginPort", 7171);

		m_confNumber[SQL_PORT] = getGlobalNumber("sqlPort", 3306);
		m_confNumber[SQL_KEEPALIVE] = getGlobalNumber("sqlKeepAlive", 0);
		m_confNumber[MYSQL_READ_TIMEOUT] = getGlobalNumber("mysqlReadTimeout", 10);
		m_confNumber[MYSQL_WRITE_TIMEOUT] = getGlobalNumber("mysqlWriteTimeout", 10);
		m_confNumber[GLOBALSAVE_H] = getGlobalNumber("globalSaveHour", 8);
		m_confNumber[GLOBALSAVE_M] = getGlobalNumber("globalSaveMinute", 0);
		m_confNumber[WORLD_ID] = getGlobalNumber("worldId", 0);
		m_confNumber[PASSWORDTYPE] = PASSWORD_TYPE_PLAIN;


		m_confBool[TRUNCATE_LOGS] = getGlobalBool("truncateLogsOnStartup", true);
		m_confBool[OPTIMIZE_DATABASE] = getGlobalBool("startupDatabaseOptimization", true);
		m_confBool[GLOBALSAVE_ENABLED] = getGlobalBool("globalSaveEnabled", true);
		m_confBool[RANDOMIZE_TILES] = getGlobalBool("randomizeTiles", true);
		m_confBool[STORE_TRASH] = getGlobalBool("storeTrash", true);
		m_confBool[EXPERIENCE_STAGES] = getGlobalBool("experienceStages", false);
		m_confBool[GUILD_HALLS] = getGlobalBool("guildHalls", false);
	}

	m_confString[MAP_AUTHOR] = getGlobalString("mapAuthor", "Unknown");
	m_confString[SERVER_NAME] = getGlobalString("serverName");
	m_confString[OWNER_NAME] = getGlobalString("ownerName");
	m_confString[OWNER_EMAIL] = getGlobalString("ownerEmail");
	m_confString[URL] = getGlobalString("url");
	m_confString[LOCATION] = getGlobalString("location");
	m_confString[MOTD] = getGlobalString("motd");
	m_confString[WORLD_TYPE] = getGlobalString("worldType", "pvp");
	m_confString[PREFIX_CHANNEL_LOGS] = getGlobalString("prefixChannelLogs", "");
	m_confString[CORES_USED] = getGlobalString("coresUsed", "-1");
	m_confString[MAILBOX_DISABLED_TOWNS] = getGlobalString("mailboxDisabledTowns", "-1");
	m_confString[HOUSE_STORAGE] = getGlobalString("houseDataStorage", "binary");


	m_confNumber[LOGIN_TRIES] = getGlobalNumber("loginTries", 3);
	m_confNumber[RETRY_TIMEOUT] = getGlobalNumber("retryTimeout", 30 * 1000);
	m_confNumber[LOGIN_TIMEOUT] = getGlobalNumber("loginTimeout", 5 * 1000);
	m_confNumber[MAX_MESSAGEBUFFER] = getGlobalNumber("maxMessageBuffer", 4);
	m_confNumber[MAX_PLAYERS] = getGlobalNumber("maxPlayers");
	m_confNumber[DEFAULT_DESPAWNRANGE] = getGlobalNumber("deSpawnRange", 2);
	m_confNumber[DEFAULT_DESPAWNRADIUS] = getGlobalNumber("deSpawnRadius", 50);
	m_confNumber[PZ_LOCKED] = getGlobalNumber("pzLocked", 60 * 1000);
	m_confNumber[HUNTING_DURATION] = getGlobalNumber("huntingDuration", 60 * 1000);
	m_confNumber[ALLOW_CLONES] = getGlobalNumber("allowClones", 0);
	m_confNumber[RATE_SPAWN_MIN] = getGlobalNumber("rateSpawnMin", 1);
	m_confNumber[RATE_SPAWN_MAX] = getGlobalNumber("rateSpawnMax", 1);
	m_confNumber[SPAWNPOS_X] = getGlobalNumber("newPlayerSpawnPosX", 100);
	m_confNumber[SPAWNPOS_Y] = getGlobalNumber("newPlayerSpawnPosY", 100);
	m_confNumber[SPAWNPOS_Z] = getGlobalNumber("newPlayerSpawnPosZ", 7);
	m_confNumber[SPAWNTOWN_ID] = getGlobalNumber("newPlayerTownId", 1);
	m_confNumber[START_LEVEL] = getGlobalNumber("newPlayerLevel", 1);
	m_confNumber[START_MAGICLEVEL] = getGlobalNumber("newPlayerMagicLevel", 0);
	m_confNumber[HOUSE_PRICE] = getGlobalNumber("housePriceEachSquare", 1000);
	m_confNumber[WHITE_SKULL_TIME] = getGlobalNumber("whiteSkullTime", 15 * 60 * 1000);
	m_confNumber[HIGHSCORES_TOP] = getGlobalNumber("highscoreDisplayPlayers", 10);
	m_confNumber[HIGHSCORES_UPDATETIME] = getGlobalNumber("updateHighscoresAfterMinutes", 60);
	m_confNumber[ACTIONS_DELAY_INTERVAL] = getGlobalNumber("timeBetweenActions", 200);
	m_confNumber[EX_ACTIONS_DELAY_INTERVAL] = getGlobalNumber("timeBetweenExActions", 1000);
	m_confNumber[CRITICAL_HIT_CHANCE] = getGlobalNumber("criticalHitChance", 5);
	m_confNumber[PROTECTION_LEVEL] = getGlobalNumber("protectionLevel", 1);
	m_confNumber[STATUSQUERY_TIMEOUT] = getGlobalNumber("statusTimeout", 5 * 60 * 1000);
	m_confNumber[LEVEL_TO_FORM_GUILD] = getGlobalNumber("levelToFormGuild", 8);
	m_confNumber[MIN_GUILDNAME] = getGlobalNumber("guildNameMinLength", 4);
	m_confNumber[MAX_GUILDNAME] = getGlobalNumber("guildNameMaxLength", 20);
	m_confNumber[LEVEL_TO_BUY_HOUSE] = getGlobalNumber("levelToBuyHouse", 1);
	m_confNumber[HOUSES_PER_ACCOUNT] = getGlobalNumber("housesPerAccount", 0);
	m_confNumber[RED_SKULL_LENGTH] = getGlobalNumber("redSkullLength", 30 * 24 * 60 * 60);
	m_confNumber[MAX_VIOLATIONCOMMENT_SIZE] = getGlobalNumber("maxViolationCommentSize", 60);
	m_confNumber[NOTATIONS_TO_BAN] = getGlobalNumber("notationsToBan", 3);
	m_confNumber[WARNINGS_TO_FINALBAN] = getGlobalNumber("warningsToFinalBan", 4);
	m_confNumber[WARNINGS_TO_DELETION] = getGlobalNumber("warningsToDeletion", 5);
	m_confNumber[BAN_LENGTH] = getGlobalNumber("banLength", 7 * 24 * 60 * 60);
	m_confNumber[KILLS_BAN_LENGTH] = getGlobalNumber("killsBanLength", 7 * 24 * 60 * 60);
	m_confNumber[FINALBAN_LENGTH] = getGlobalNumber("finalBanLength", 30 * 24 * 60 * 60);
	m_confNumber[IPBANISHMENT_LENGTH] = getGlobalNumber("ipBanishmentLength", 1 * 24 * 60 * 60);
	m_confNumber[MAX_PLAYER_SUMMONS] = getGlobalNumber("maxPlayerSummons", 2);
	m_confNumber[FIELD_OWNERSHIP] = getGlobalNumber("fieldOwnershipDuration", 5 * 1000);
	m_confNumber[LOGIN_PROTECTION] = getGlobalNumber("loginProtectionPeriod", 10 * 1000);
	m_confNumber[PLAYER_DEEPNESS] = getGlobalNumber("playerQueryDeepness", 1);
	m_confNumber[STAIRHOP_DELAY] = getGlobalNumber("stairhopDelay", 2 * 1000);
	m_confNumber[RATE_STAMINA_LOSS] = getGlobalNumber("rateStaminaLoss", 1);
	m_confNumber[STAMINA_LIMIT_TOP] = getGlobalNumber("staminaRatingLimitTop", 55 * 60);
	m_confNumber[STAMINA_LIMIT_BOTTOM] = getGlobalNumber("staminaRatingLimitBottom", 14 * 60);
	m_confNumber[BLESS_REDUCTION_BASE] = getGlobalNumber("blessingReductionBase", 30);
	m_confNumber[BLESS_REDUCTION_DECREAMENT] = getGlobalNumber("blessingReductionDecreament", 5);
	m_confNumber[BLESS_REDUCTION] = getGlobalNumber("eachBlessReduction", 8);
	m_confNumber[NICE_LEVEL] = getGlobalNumber("niceLevel", 5);
	m_confNumber[EXPERIENCE_COLOR] = getGlobalNumber("gainExperienceColor", TEXTCOLOR_WHITE);
	m_confNumber[GUILD_PREMIUM_DAYS] = getGlobalNumber("premiumDaysToFormGuild", 0);
	m_confNumber[PUSH_CREATURE_DELAY] = getGlobalNumber("pushCreatureDelay", 2 * 1000);
	m_confNumber[DEATH_CONTAINER] = getGlobalNumber("deathContainerId", 1987);
	m_confNumber[MAXIMUM_DOOR_LEVEL] = getGlobalNumber("maximumDoorLevel", 500);
	m_confNumber[DEATH_ASSISTS] = getGlobalNumber("deathAssistCount", 1);
	m_confNumber[RED_DAILY_LIMIT] = getGlobalNumber("dailyFragsToRedSkull", 3);
	m_confNumber[RED_WEEKLY_LIMIT] = getGlobalNumber("weeklyFragsToRedSkull", 5);
	m_confNumber[RED_MONTHLY_LIMIT] = getGlobalNumber("monthlyFragsToRedSkull", 10);
	m_confNumber[BAN_DAILY_LIMIT] = getGlobalNumber("dailyFragsToBanishment", m_confNumber[RED_DAILY_LIMIT]);
	m_confNumber[BAN_WEEKLY_LIMIT] = getGlobalNumber("weeklyFragsToBanishment", m_confNumber[RED_WEEKLY_LIMIT]);
	m_confNumber[BAN_MONTHLY_LIMIT] = getGlobalNumber("monthlyFragsToBanishment", m_confNumber[RED_MONTHLY_LIMIT]);
	m_confNumber[DEATHLIST_REQUIRED_TIME] = getGlobalNumber("deathListRequiredTime", 1 * 60 * 1000);
	m_confNumber[TILE_LIMIT] = getGlobalNumber("tileLimit", 0);
	m_confNumber[PROTECTION_TILE_LIMIT] = getGlobalNumber("protectionTileLimit", 0);
	m_confNumber[HOUSE_TILE_LIMIT] = getGlobalNumber("houseTileLimit", 0);
	m_confNumber[SQUARE_COLOR] = getGlobalNumber("squareColor", 0);
	m_confNumber[LOOT_MESSAGE] = getGlobalNumber("monsterLootMessage", 3);
	m_confNumber[LOOT_MESSAGE_TYPE] = getGlobalNumber("monsterLootMessageType", 25);
	m_confNumber[NAME_REPORT_TYPE] = getGlobalNumber("violationNameReportActionType", 2);
	m_confNumber[HOUSE_CLEAN_OLD] = getGlobalNumber("houseCleanOld", 0);
	m_confNumber[FIST_BASE_ATTACK] = getGlobalNumber("fistBaseAttack", 7);
	m_confNumber[VIPLIST_DEFAULT_LIMIT] = getGlobalNumber("vipListDefaultLimit", 20);
	m_confNumber[VIPLIST_DEFAULT_PREMIUM_LIMIT] = getGlobalNumber("vipListDefaultPremiumLimit", 100);
	m_confNumber[TRADE_LIMIT] = getGlobalNumber("tradeLimit", 100);
	m_confNumber[MAIL_ATTEMPTS] = getGlobalNumber("mailMaxAttempts", 20);
	m_confNumber[MAIL_BLOCK] = getGlobalNumber("mailBlockPeriod", 3600000);
	m_confNumber[MAIL_ATTEMPTS_FADE] = getGlobalNumber("mailAttemptsFadeTime", 600000);
	m_confNumber[DEFAULT_DEPOT_SIZE_PREMIUM] = getGlobalNumber("defaultDepotSizePremium", 2000);
	m_confNumber[DEFAULT_DEPOT_SIZE] = getGlobalNumber("defaultDepotSize", 2000);


	m_confDouble[RATE_EXPERIENCE] = getGlobalDouble("rateExperience", 1);
	m_confDouble[RATE_SKILL] = getGlobalDouble("rateSkill", 1);
	m_confDouble[RATE_MAGIC] = getGlobalDouble("rateMagic", 1);
	m_confDouble[RATE_LOOT] = getGlobalDouble("rateLoot", 1);
	m_confDouble[RATE_PVP_EXPERIENCE] = getGlobalDouble("rateExperienceFromPlayers", 0);
	m_confDouble[EFP_MIN_THRESHOLD] = getGlobalDouble("minLevelThresholdForKilledPlayer", 0.9f);
	m_confDouble[EFP_MAX_THRESHOLD] = getGlobalDouble("maxLevelThresholdForKilledPlayer", 1.1f);
	m_confDouble[CRITICAL_HIT_MUL] = getGlobalDouble("criticalHitMultiplier", 1);
	m_confDouble[RATE_STAMINA_GAIN] = getGlobalDouble("rateStaminaGain", 3);
	m_confDouble[RATE_STAMINA_THRESHOLD] = getGlobalDouble("rateStaminaThresholdGain", 12);
	m_confDouble[RATE_STAMINA_ABOVE] = getGlobalDouble("rateStaminaAboveNormal", 1.5f);
	m_confDouble[RATE_STAMINA_UNDER] = getGlobalDouble("rateStaminaUnderNormal", 0.5f);
	m_confDouble[FORMULA_LEVEL] = getGlobalDouble("formulaLevel", 5.0);
	m_confDouble[FORMULA_MAGIC] = getGlobalDouble("formulaMagic", 1.0);
	m_confDouble[RATE_MONSTER_HEALTH] = getGlobalDouble("rateMonsterHealth", 1);
	m_confDouble[RATE_MONSTER_MANA] = getGlobalDouble("rateMonsterMana", 1);
	m_confDouble[RATE_MONSTER_ATTACK] = getGlobalDouble("rateMonsterAttack", 1);
	m_confDouble[RATE_MONSTER_DEFENSE] = getGlobalDouble("rateMonsterDefense", 1);


	m_confBool[ACCOUNT_MANAGER] = getGlobalBool("accountManager", true);
	m_confBool[NAMELOCK_MANAGER] = getGlobalBool("namelockManager", false);
	m_confBool[START_CHOOSEVOC] = getGlobalBool("newPlayerChooseVoc", false);
	m_confBool[ON_OR_OFF_CHARLIST] = getGlobalBool("displayOnOrOffAtCharlist", false);
	m_confBool[ALLOW_CHANGEOUTFIT] = getGlobalBool("allowChangeOutfit", true);
	m_confBool[ONE_PLAYER_ON_ACCOUNT] = getGlobalBool("onePlayerOnlinePerAccount", true);
	m_confBool[CANNOT_ATTACK_SAME_LOOKFEET] = getGlobalBool("noDamageToSameLookfeet", false);
	m_confBool[AIMBOT_HOTKEY_ENABLED] = getGlobalBool("hotkeyAimbotEnabled", true);
	m_confBool[REMOVE_WEAPON_AMMO] = getGlobalBool("removeWeaponAmmunition", true);
	m_confBool[REMOVE_WEAPON_CHARGES] = getGlobalBool("removeWeaponCharges", true);
	m_confBool[REMOVE_RUNE_CHARGES] = getGlobalBool("removeRuneCharges", true);
	m_confBool[SHUTDOWN_AT_GLOBALSAVE] = getGlobalBool("shutdownAtGlobalSave", false);
	m_confBool[CLEAN_MAP_AT_GLOBALSAVE] = getGlobalBool("cleanMapAtGlobalSave", true);
	m_confBool[FREE_PREMIUM] = getGlobalBool("freePremium", false);
	m_confBool[BROADCAST_BANISHMENTS] = getGlobalBool("broadcastBanishments", true);
	m_confBool[GENERATE_ACCOUNT_NUMBER] = getGlobalBool("generateAccountNumber", true);
	m_confBool[INGAME_GUILD_MANAGEMENT] = getGlobalBool("ingameGuildManagement", true);
	m_confBool[HOUSE_BUY_AND_SELL] = getGlobalBool("buyableAndSellableHouses", true);
	m_confBool[REPLACE_KICK_ON_LOGIN] = getGlobalBool("replaceKickOnLogin", true);
	m_confBool[HOUSE_NEED_PREMIUM] = getGlobalBool("houseNeedPremium", true);
	m_confBool[HOUSE_RENTASPRICE] = getGlobalBool("houseRentAsPrice", false);
	m_confBool[HOUSE_PRICEASRENT] = getGlobalBool("housePriceAsRent", false);
	m_confBool[BANK_SYSTEM] = getGlobalBool("bankSystem", true);
	m_confBool[PREMIUM_FOR_PROMOTION] = getGlobalBool("premiumForPromotion", true);
	m_confBool[INIT_PREMIUM_UPDATE] = getGlobalBool("updatePremiumStateAtStartup", true);
	m_confBool[SHOW_HEALING_DAMAGE] = getGlobalBool("showHealingDamage", false);
	m_confBool[TELEPORT_SUMMONS] = getGlobalBool("teleportAllSummons", false);
	m_confBool[TELEPORT_PLAYER_SUMMONS] = getGlobalBool("teleportPlayerSummons", false);
	m_confBool[PVP_TILE_IGNORE_PROTECTION] = getGlobalBool("pvpTileIgnoreLevelAndVocationProtection", true);
	m_confBool[DISPLAY_CRITICAL_HIT] = getGlobalBool("displayCriticalHitNotify", false);
	m_confBool[ADVANCING_SKILL_LEVEL] = getGlobalBool("displaySkillLevelOnAdvance", false);
	m_confBool[CLEAN_PROTECTED_ZONES] = getGlobalBool("cleanProtectedZones", true);
	m_confBool[SPELL_NAME_INSTEAD_WORDS] = getGlobalBool("spellNameInsteadOfWords", false);
	m_confBool[EMOTE_SPELLS] = getGlobalBool("emoteSpells", false);
	m_confBool[SAVE_GLOBAL_STORAGE] = getGlobalBool("saveGlobalStorage", true);
	m_confBool[BLESSINGS] = getGlobalBool("blessings", true);
	m_confBool[BLESSING_ONLY_PREMIUM] = getGlobalBool("blessingOnlyPremium", true);
	m_confBool[BED_REQUIRE_PREMIUM] = getGlobalBool("bedsRequirePremium", true);
	m_confBool[ALLOW_CHANGECOLORS] = getGlobalBool("allowChangeColors", true);
	m_confBool[STOP_ATTACK_AT_EXIT] = getGlobalBool("stopAttackingAtExit", false);
	m_confBool[DISABLE_OUTFITS_PRIVILEGED] = getGlobalBool("disableOutfitsForPrivilegedPlayers", false);
	m_confBool[OLD_CONDITION_ACCURACY] = getGlobalBool("oldConditionAccuracy", false);
	m_confBool[TRACER_BOX] = getGlobalBool("promptExceptionTracerErrorBox", true);
	m_confBool[STORE_DIRECTION] = getGlobalBool("storePlayerDirection", false);
	m_confBool[DISPLAY_LOGGING] = getGlobalBool("displayPlayersLogging", true);
	m_confBool[STAMINA_BONUS_PREMIUM] = getGlobalBool("staminaThresholdOnlyPremium", true);
	m_confBool[BAN_UNKNOWN_BYTES] = getGlobalBool("autoBanishUnknownBytes", false);
	m_confBool[ALLOW_CHANGEADDONS] = getGlobalBool("allowChangeAddons", true);
	m_confBool[GHOST_INVISIBLE_EFFECT] = getGlobalBool("ghostModeInvisibleEffect", false);
	m_confBool[SHOW_HEALING_DAMAGE_MONSTER] = getGlobalBool("showHealingDamageForMonsters", false);
	m_confBool[CHECK_CORPSE_OWNER] = getGlobalBool("checkCorpseOwner ", true);
	m_confBool[BUFFER_SPELL_FAILURE] = getGlobalBool("bufferMutedOnSpellFailure", false);
	m_confBool[PREMIUM_SKIP_WAIT] = getGlobalBool("premiumPlayerSkipWaitList", false);
	m_confBool[DEATH_LIST] = getGlobalBool("deathListEnabled", true);
	m_confBool[GHOST_SPELL_EFFECTS] = getGlobalBool("ghostModeSpellEffects", true);
	m_confBool[PVPZONE_ADDMANASPENT] = getGlobalBool("addManaSpentInPvPZone", true);
	m_confBool[USE_FRAG_HANDLER] = getGlobalBool("useFragHandler", true);
	m_confBool[ALLOW_FIGHTBACK] = getGlobalBool("allowFightback", true);
	m_confBool[ALLOW_FIGHTBACK] = getGlobalBool("allowFightback", true);
	m_confBool[VIPLIST_PER_PLAYER] = getGlobalBool("separateViplistPerCharacter", false);
	m_confBool[ADDONS_PREMIUM] = getGlobalBool("addonsOnlyPremium", true);
	m_confBool[SKIP_ITEMS_VERSION] = getGlobalBool("skipItemsVersionCheck", true);
	m_confBool[UNIFIED_SPELLS] = getGlobalBool("unifiedSpells", true);
	m_confBool[ALLOW_BLOCK_SPAWN] = getGlobalBool("allowBlockSpawn", true);
	m_confBool[HOUSE_PROTECTION] = getGlobalBool("houseProtection", true);
	m_confBool[USE_RUNE_REQUIREMENTS] = getGlobalBool("useRunesRequirements", true);
	m_confBool[HOUSE_SKIP_INIT_RENT] = getGlobalBool("houseSkipInitialRent", true);
	m_confBool[DAEMONIZE] = getGlobalBool("daemonize", false);
	m_confBool[USE_CAPACITY] = getGlobalBool("useCapacity", true);
	m_confBool[CLASSIC_SPELLS] = getGlobalBool("classicSpells", true);
	m_confBool[MULTIPLE_NAME] = getGlobalBool("multipleNames", false);
	m_confBool[SAVE_PLAYER_DATA] = getGlobalBool("savePlayerData", true);
	m_confBool[CLOSE_INSTANCE_ON_SHUTDOWN] = getGlobalBool("closeInstanceOnShutdown", true);

	m_loaded = true;
	return true;
}

bool ConfigManager::reload()
{
	if(!m_loaded)
		return false;

	return load();
}

const std::string& ConfigManager::getString(uint32_t _what) const
{
	if((m_loaded && _what < LAST_STRING_CONFIG) || _what <= CONFIG_FILE)
		return m_confString[_what];

	if(!m_startup)
		std::clog << "[Warning - ConfigManager::getString] " << _what << std::endl;

	return m_confString[DUMMY_STR];
}

bool ConfigManager::getBool(uint32_t _what) const
{
	if(m_loaded && _what < LAST_BOOL_CONFIG)
		return m_confBool[_what];

	if(!m_startup)
		std::clog << "[Warning - ConfigManager::getBool] " << _what << std::endl;

	return false;
}

int32_t ConfigManager::getNumber(uint32_t _what) const
{
	if(m_loaded && _what < LAST_NUMBER_CONFIG)
		return m_confNumber[_what];

	if(!m_startup)
		std::clog << "[Warning - ConfigManager::getNumber] " << _what << std::endl;

	return 0;
}

double ConfigManager::getDouble(uint32_t _what) const
{
	if(m_loaded && _what < LAST_DOUBLE_CONFIG)
		return m_confDouble[_what];

	if(!m_startup)
		std::clog << "[Warning - ConfigManager::getDouble] " << _what << std::endl;

	return 0;
}

bool ConfigManager::setString(uint32_t _what, const std::string& _value)
{
	if(_what < LAST_STRING_CONFIG)
	{
		m_confString[_what] = _value;
		return true;
	}

	std::clog << "[Warning - ConfigManager::setString] " << _what << std::endl;
	return false;
}

bool ConfigManager::setNumber(uint32_t _what, int32_t _value)
{
	if(_what < LAST_NUMBER_CONFIG)
	{
		m_confNumber[_what] = _value;
		return true;
	}

	std::clog << "[Warning - ConfigManager::setNumber] " << _what << std::endl;
	return false;
}

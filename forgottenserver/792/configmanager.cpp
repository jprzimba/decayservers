//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//////////////////////////////////////////////////////////////////////
#include "otpch.h"

#include "definitions.h"
#include "configmanager.h"
#include "tools.h"

#include <iostream>
#include <stdexcept>

ConfigManager::ConfigManager()
{
	m_loaded = false;
}

ConfigManager::~ConfigManager()
{
	//
}

bool ConfigManager::load()
{
	lua_State* L = luaL_newstate();
	if(!L)
		throw std::runtime_error("Failed to allocate memory");

	luaL_openlibs(L);

	if(luaL_dofile(L, "config.lua"))
	{
		std::cout << "[Error - ConfigManager::load] " << lua_tostring(L, -1) << std::endl;
		lua_close(L);
		return false;
	}

	//parse config
	if(!m_loaded) //info that must be loaded one time (unless we reset the modules involved)
	{
		string[IP] = getGlobalString(L, "ip", "127.0.0.1");
		string[MAP_NAME] = getGlobalString(L, "mapName", "forgotten");
		string[MAP_AUTHOR] = getGlobalString(L, "mapAuthor", "Unknown");
		string[HOUSE_RENT_PERIOD] = getGlobalString(L, "houseRentPeriod", "monthly");
		string[MYSQL_HOST] = getGlobalString(L, "mysqlHost", "localhost");
		string[MYSQL_USER] = getGlobalString(L, "mysqlUser", "root");
		string[MYSQL_PASS] = getGlobalString(L, "mysqlPass", "");
		string[MYSQL_DB] = getGlobalString(L, "mysqlDatabase", "theforgottenserver");
		string[PASSWORDTYPE] = getGlobalString(L, "passwordType", "plain");
		integer[PASSWORD_TYPE] = PASSWORD_TYPE_PLAIN;
	
		integer[PORT] = getGlobalNumber(L, "port", 7171);
		integer[SQL_PORT] = getGlobalNumber(L, "mysqlPort", 3306);
		integer[SERVERSAVE_H] = getGlobalNumber(L, "serverSaveHour", 3);

		boolean[SERVERSAVE_ENABLED] = getGlobalBoolean(L, "serverSaveEnabled", true);
		boolean[SAVE_GLOBAL_STORAGE] = getGlobalBoolean(L, "saveGlobalStorage", false);
		boolean[OPTIMIZE_DATABASE] = getGlobalBoolean(L, "startupDatabaseOptimization", true);
		boolean[INGAME_GUILD_SYSTEM] = getGlobalBoolean(L, "ingameGuildSystem", true);
	}

	string[LOGIN_MSG] = getGlobalString(L, "loginMessage", "Welcome to the Forgotten Server!");
	string[SERVER_NAME] = getGlobalString(L, "serverName");
	string[OWNER_NAME] = getGlobalString(L, "ownerName");
	string[OWNER_EMAIL] = getGlobalString(L, "ownerEmail");
	string[URL] = getGlobalString(L, "url");
	string[LOCATION] = getGlobalString(L, "location");
	string[MOTD] = getGlobalString(L, "motd");
	string[WORLD_TYPE] = getGlobalString(L, "worldType", "pvp");
	string[MAP_STORAGE_TYPE] = getGlobalString(L, "mapStorageType", "relational");
	string[DEFAULT_PRIORITY] = getGlobalString(L, "defaultPriority", "high");


	integer[LOGIN_TRIES] = getGlobalNumber(L, "loginTries", 3);
	integer[RETRY_TIMEOUT] = getGlobalNumber(L, "retryTimeout", 30 * 1000);
	integer[LOGIN_TIMEOUT] = getGlobalNumber(L, "loginTimeout", 5 * 1000);
	integer[MAX_PLAYERS] = getGlobalNumber(L, "maxPlayers", 0);
	integer[PZ_LOCKED] = getGlobalNumber(L, "pzLocked", 0);
	integer[DEFAULT_DESPAWNRANGE] = getGlobalNumber(L, "deSpawnRange", 2);
	integer[DEFAULT_DESPAWNRADIUS] = getGlobalNumber(L, "deSpawnRadius", 50);
	integer[RATE_EXPERIENCE] = getGlobalNumber(L, "rateExperience", 1);
	integer[RATE_SKILL] = getGlobalNumber(L, "rateSkill", 1);
	integer[RATE_LOOT] = getGlobalNumber(L, "rateLoot", 1);
	integer[RATE_MAGIC] = getGlobalNumber(L, "rateMagic", 1);
	integer[RATE_SPAWN] = getGlobalNumber(L, "rateSpawn", 1);
	integer[SPAWNPOS_X] = getGlobalNumber(L, "newPlayerSpawnPosX", 100);
	integer[SPAWNPOS_Y] = getGlobalNumber(L, "newPlayerSpawnPosY", 100);
	integer[SPAWNPOS_Z] = getGlobalNumber(L, "newPlayerSpawnPosZ", 7);
	integer[SPAWNTOWN_ID] = getGlobalNumber(L, "newPlayerTownId", 1);
	integer[START_LEVEL] = getGlobalNumber(L, "newPlayerLevel", 1);
	integer[START_MAGICLEVEL] = getGlobalNumber(L, "newPlayerMagicLevel", 0);
	integer[HOUSE_PRICE] = getGlobalNumber(L, "housePriceEachSQM", 1000);
	integer[KILLS_TO_RED] = getGlobalNumber(L, "killsToRedSkull", 3);
	integer[KILLS_TO_BAN] = getGlobalNumber(L, "killsToBan", 5);
	integer[HIGHSCORES_TOP] = getGlobalNumber(L, "highscoreDisplayPlayers", 10);
	integer[HIGHSCORES_UPDATETIME] = getGlobalNumber(L, "updateHighscoresAfterMinutes", 60);
	integer[BAN_DAYS] = getGlobalNumber(L, "banDays", 7);
	integer[FINAL_BAN_DAYS] = getGlobalNumber(L, "finalBanDays", 30);
	integer[FRAG_TIME] = getGlobalNumber(L, "timeToDecreaseFrags", 24 * 60 * 60 * 1000);
	integer[WHITE_SKULL_TIME] = getGlobalNumber(L, "whiteSkullTime", 15 * 60 * 1000);
	integer[ACTIONS_DELAY_INTERVAL] = getGlobalNumber(L, "timeBetweenActions", 200);
	integer[EX_ACTIONS_DELAY_INTERVAL] = getGlobalNumber(L, "timeBetweenExActions", 1000);
	integer[MAX_MESSAGEBUFFER] = getGlobalNumber(L, "maxMessageBuffer", 4);
	integer[CRITICAL_HIT_CHANCE] = getGlobalNumber(L, "criticalHitChance", 5);
	integer[KICK_AFTER_MINUTES] = getGlobalNumber(L, "kickIdlePlayerAfterMinutes", 15);
	integer[DEATH_LOSE_PERCENT] = getGlobalNumber(L, "deathLosePercent", 10);
	integer[STATUSQUERY_TIMEOUT] = getGlobalNumber(L, "statusTimeout", 5 * 60 * 1000);
	integer[PROTECTION_LEVEL] = getGlobalNumber(L, "protectionLevel", 1);
	integer[AUTO_SAVE_EACH_MINUTES] = getGlobalNumber(L, "autoSaveEachMinutes", 0);
	integer[LEVEL_TO_CREATE_GUILD] = getGlobalNumber(L, "levelToCreateGuild", 8);
	integer[MIN_GUILD_NAME] = getGlobalNumber(L, "minGuildNameLength", 4);
	integer[MAX_GUILD_NAME] = getGlobalNumber(L, "maxGuildNameLength", 20);
	integer[SUPRISEBAG_PERCENT] = getGlobalNumber(L, "surpriseBagPercent", 0);


	boolean[CANNOT_ATTACK_SAME_LOOKFEET] = getGlobalBoolean(L, "noDamageToSameLookfeet", false);
	boolean[AIMBOT_HOTKEY_ENABLED] = getGlobalBoolean(L, "hotkeyAimbotEnabled", true);
	boolean[REMOVE_AMMO] = getGlobalBoolean(L, "removeAmmoWhenUsingDistanceWeapon", true);
	boolean[REMOVE_RUNE_CHARGES] = getGlobalBoolean(L, "removeChargesFromRunes", true);
	boolean[ONE_PLAYER_ON_ACCOUNT] = getGlobalBoolean(L, "onePlayerOnlinePerAccount", true);
	boolean[RANDOMIZE_TILES] = getGlobalBoolean(L, "randomizeTiles", true);
	boolean[EXPERIENCE_FROM_PLAYERS] = getGlobalBoolean(L, "experienceByKillingPlayers", false);
	boolean[SHUTDOWN_AT_SERVERSAVE] = getGlobalBoolean(L, "shutdownAtServerSave", false);
	boolean[CLEAN_MAP_AT_SERVERSAVE] = getGlobalBoolean(L, "cleanMapAtServerSave", true);
	boolean[BROADCAST_BANISHMENTS] = getGlobalBoolean(L, "broadcastBanishments", true);
	boolean[GENERATE_ACCOUNT_NUMBER] = getGlobalBoolean(L, "generateAccountNumber", true);
	boolean[ACCOUNT_MANAGER] = getGlobalBoolean(L, "accountManager", true);
	boolean[START_CHOOSEVOC] = getGlobalBoolean(L, "newPlayerChooseVoc", false);
	boolean[ALLOW_CLONES] = getGlobalBoolean(L, "allowClones", false);
	boolean[FREE_PREMIUM] = getGlobalBoolean(L, "freePremium", false);
	boolean[ON_OR_OFF_CHARLIST] = getGlobalBoolean(L, "displayOnOrOffAtCharlist", false);
	boolean[ALLOW_CHANGEOUTFIT] = getGlobalBoolean(L, "allowChangeOutfit", true);
	boolean[CLEAN_PZ] = getGlobalBoolean(L, "cleanProtectedZones", true);
	boolean[ENABLE_RULE_VIOLATION_REPORTS] = getGlobalBoolean(L, "enableRuleViolationReports", false);
	boolean[SURPRISE_BAGS] = getGlobalBoolean(L, "monstersDropSurpriseBags", false);
	boolean[SPELL_NAME_INSTEAD_WORDS] = getGlobalBoolean(L, "spellNameInsteadOfWords", false);
	boolean[EMOTE_SPELLS] = getGlobalBoolean(L, "emoteSpells", false);
	boolean[EXPERIENCE_STAGES] = getGlobalBoolean(L, "experienceStages", false);

	m_loaded = true;
	lua_close(L);
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
	if(m_loaded && _what < LAST_STRING_CONFIG)
		return string[_what];
	else
	{
		std::cout << "Warning: [ConfigManager::getString] " << _what << std::endl;
		return string[DUMMY_STR];
	}
}

int32_t ConfigManager::getNumber(uint32_t _what) const
{
	if(m_loaded && _what < LAST_INTEGER_CONFIG)
		return integer[_what];
	else
	{
		std::cout << "Warning: [ConfigManager::getNumber] " << _what << std::endl;
		return 0;
	}
}
bool ConfigManager::setNumber(uint32_t _what, int32_t _value)
{
	if(m_loaded && _what < LAST_INTEGER_CONFIG)
	{
		integer[_what] = _value;
		return true;
	}
	else
	{
		std::cout << "Warning: [ConfigManager::setNumber] " << _what << std::endl;
		return false;
	}
}

std::string ConfigManager::getGlobalString(lua_State* _L, const std::string& _identifier, const std::string& _default)
{
	lua_getglobal(_L, _identifier.c_str());

	if(!lua_isstring(_L, -1))
		return _default;

	int32_t len = (int32_t)lua_strlen(_L, -1);
	std::string ret(lua_tostring(_L, -1), len);
	lua_pop(_L,1);

	return ret;
}

int32_t ConfigManager::getGlobalNumber(lua_State* _L, const std::string& _identifier, const int32_t _default)
{
	lua_getglobal(_L, _identifier.c_str());

	if(!lua_isnumber(_L, -1))
		return _default;

	int32_t val = (int32_t)lua_tonumber(_L, -1);
	lua_pop(_L,1);

	return val;
}

std::string ConfigManager::getGlobalStringField (lua_State* _L, const std::string& _identifier, const int32_t _key, const std::string& _default) {
	lua_getglobal(_L, _identifier.c_str());

	lua_pushnumber(_L, _key);
	lua_gettable(_L, -2);  /* get table[key] */
	if(!lua_isstring(_L, -1))
		return _default;
	std::string result = lua_tostring(_L, -1);
	lua_pop(_L, 2);  /* remove number and key*/
	return result;
}

bool ConfigManager::getBool(uint32_t _what) const
{
	if(m_loaded && _what < LAST_BOOLEAN_CONFIG)
		return boolean[_what];
	else
	{
		std::cout << "Warning: [ConfigManager::getBool] " << _what << std::endl;
		return false;
	}
}

bool ConfigManager::getGlobalBoolean(lua_State* L, const char* identifier, const bool defaultValue)
{
	lua_getglobal(L, identifier);
	if (!lua_isboolean(L, -1)) {
		if (!lua_isstring(L, -1)) {
			return defaultValue;
		}

		size_t len = lua_strlen(L, -1);
		std::string ret(lua_tostring(L, -1), len);
		lua_pop(L, 1);
		return booleanString(ret);
	}

	int val = lua_toboolean(L, -1);
	lua_pop(L, 1);
	return val != 0;
}

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

#ifndef _CONFIG_MANAGER_H
#define _CONFIG_MANAGER_H

#include <string>

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

class ConfigManager
{
	public:
		ConfigManager();
		~ConfigManager();

		enum boolean_config_t
		{
			INGAME_GUILD_SYSTEM = 0,
			FREE_PREMIUM,
			ALLOW_CLONES,
			ACCOUNT_MANAGER,
			START_CHOOSEVOC,
			ON_OR_OFF_CHARLIST,
			ALLOW_CHANGEOUTFIT,
			ONE_PLAYER_ON_ACCOUNT,
			GENERATE_ACCOUNT_NUMBER,
			BROADCAST_BANISHMENTS,
			SHUTDOWN_AT_SERVERSAVE,
			CLEAN_MAP_AT_SERVERSAVE,
			EXPERIENCE_FROM_PLAYERS,
			RANDOMIZE_TILES,
			REMOVE_AMMO,
			REMOVE_RUNE_CHARGES,
			CANNOT_ATTACK_SAME_LOOKFEET,
			AIMBOT_HOTKEY_ENABLED,
			OPTIMIZE_DATABASE,
			CLEAN_PZ,
			SAVE_GLOBAL_STORAGE,
			SERVERSAVE_ENABLED,
			ENABLE_RULE_VIOLATION_REPORTS,
			SPELL_NAME_INSTEAD_WORDS,
			EMOTE_SPELLS,
			EXPERIENCE_STAGES,
			REPLACE_KICK_ON_LOGIN,
			STOP_ATTACK_AT_EXIT,
			BUFFER_SPELL_FAILURE,
			LAST_BOOLEAN_CONFIG /* this must be the last one */
		};

		enum string_config_t 
		{
			DUMMY_STR = 0,
			MAP_NAME,
			HOUSE_RENT_PERIOD,
			LOGIN_MSG,
			FIRST_MSG,
			SERVER_NAME,
			OWNER_NAME,
			OWNER_EMAIL,
			URL,
			LOCATION,
			IP,
			MOTD,
			WORLD_TYPE,
			MYSQL_HOST,
			MYSQL_USER,
			MYSQL_PASS,
			MYSQL_DB,
			DEFAULT_PRIORITY,
			PASSWORDTYPE,
			MAP_AUTHOR,
			MAP_STORAGE_TYPE,
			SQLITE_DB,
			#ifdef MULTI_SQL_DRIVERS
			SQL_TYPE,
			#endif
			LAST_STRING_CONFIG /* this must be the last one */
		};

		enum integer_config_t
		{
			LOGIN_TRIES = 0,
			RETRY_TIMEOUT,
			LOGIN_TIMEOUT,
			PORT,
			SQL_PORT,
			MAX_PLAYERS,
			PZ_LOCKED,
			DEFAULT_DESPAWNRANGE,
			DEFAULT_DESPAWNRADIUS,
			RATE_EXPERIENCE,
			RATE_SKILL,
			RATE_LOOT,
			RATE_MAGIC,
			RATE_SPAWN,
			SPAWNPOS_X,
			SPAWNPOS_Y,
			SPAWNPOS_Z,
			SPAWNTOWN_ID,
			SERVERSAVE_H,
			START_LEVEL,
			START_MAGICLEVEL,
			HOUSE_PRICE,
			KILLS_TO_RED,
			KILLS_TO_BAN,
			HIGHSCORES_TOP,
			MAX_MESSAGEBUFFER,
			HIGHSCORES_UPDATETIME,
			ACTIONS_DELAY_INTERVAL,
			EX_ACTIONS_DELAY_INTERVAL,
			CRITICAL_HIT_CHANCE,
			KICK_AFTER_MINUTES,
			PROTECTION_LEVEL,
			DEATH_LOSE_PERCENT,
			PASSWORD_TYPE,
			STATUSQUERY_TIMEOUT,
			FRAG_TIME,
			WHITE_SKULL_TIME,
			BAN_DAYS,
			FINAL_BAN_DAYS,
			AUTO_SAVE_EACH_MINUTES,
			LEVEL_TO_CREATE_GUILD,
			MIN_GUILD_NAME,
			MAX_GUILD_NAME,
			HOUSES_PER_ACCOUNT,
			VIPLIST_DEFAULT_LIMIT,
			VIPLIST_DEFAULT_PREMIUM_LIMIT,
			DEFAULT_DEPOT_SIZE_PREMIUM,
			DEFAULT_DEPOT_SIZE,
			STAMINA_AMOUNT,
			STAMINA_LOST_MONSTER,
			LAST_INTEGER_CONFIG /* this must be the last one */
		};

		bool load();
		bool reload();
		bool isLoaded() const {return m_loaded;}

		const std::string& getString(uint32_t _what) const;
		int32_t getNumber(uint32_t _what) const;
		bool setNumber(uint32_t _what, int32_t _value);
		bool getBool(uint32_t _what) const;

	private:
		std::string getGlobalString(lua_State* _L, const std::string& _identifier, const std::string& _default="");
		int32_t getGlobalNumber(lua_State* _L, const std::string& _identifier, const int32_t _default=0);
		std::string getGlobalStringField(lua_State* _L, const std::string& _identifier, const int32_t _key, const std::string& _default="");
		static bool getGlobalBool(lua_State* L, const char* identifier, const bool defaultValue);

		bool m_loaded;
		std::string string[LAST_STRING_CONFIG];
		int32_t integer[LAST_INTEGER_CONFIG];
		bool boolean[LAST_BOOLEAN_CONFIG];
};

#endif /* _CONFIG_MANAGER_H */

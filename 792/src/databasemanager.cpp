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
#include "enums.h"
#include <iostream>

#include "databasemanager.h"
#include "tools.h"

#include "ban.h"

#include "configmanager.h"
extern ConfigManager g_config;

bool DatabaseManager::optimizeTables()
{
	Database* db = Database::getInstance();
	DBQuery query;

	query << "SELECT `TABLE_NAME` FROM `information_schema`.`TABLES` WHERE `TABLE_SCHEMA` = " << db->escapeString(g_config.getString(ConfigManager::MYSQL_DB)) << " AND `DATA_FREE` > 0;";
	DBResult* result = db->storeQuery(query.str());
	
	if(!result)
		return false;

	do
	{
		std::string tableName = result->getDataString("TABLE_NAME");
		std::clog << "> Optimizing table " << tableName << "..." << std::flush;
		query.str("");
		query << "OPTIMIZE TABLE `" << tableName << "`;";
		if(db->executeQuery(query.str()))
			std::clog << " [success]" << std::endl;
		else
			std::clog << " [failed]" << std::endl;
	}
	
	while(result->next());
		db->freeResult(result);
		
	return true;
}

bool DatabaseManager::tableExists(std::string tableName)
{
	Database* db = Database::getInstance();
	DBQuery query;
	query << "SELECT `TABLE_NAME` FROM `information_schema`.`tables` WHERE `TABLE_SCHEMA` = " << db->escapeString(g_config.getString(ConfigManager::MYSQL_DB)) << " AND `TABLE_NAME` = " << db->escapeString(tableName) << ";";
	DBResult* result = db->storeQuery(query.str());
	
	if(!result)
		return false;

	db->freeResult(result);
	return true;
}

bool DatabaseManager::isDatabaseSetup()
{
	Database* db = Database::getInstance();
	DBQuery query;
	query << "SELECT `TABLE_NAME` FROM `information_schema`.`tables` WHERE `TABLE_SCHEMA` = " << db->escapeString(g_config.getString(ConfigManager::MYSQL_DB)) << ";";
	DBResult* result = db->storeQuery(query.str());
	
	if(!result)
		return false;

	db->freeResult(result);
	return true;
}

int32_t DatabaseManager::getDatabaseVersion()
{
	if(!tableExists("server_config"))
	{
		Database* db = Database::getInstance();
		db->executeQuery("CREATE TABLE `server_config` (`config` VARCHAR(50) NOT NULL, `value` VARCHAR(256) NOT NULL DEFAULT '', UNIQUE(`config`)) ENGINE = InnoDB;");
		db->executeQuery("INSERT INTO `server_config` VALUES ('db_version', 0);");
		return 0;
	}

	int32_t version = 0;
	if(getDatabaseConfig("db_version", version))
		return version;

	return -1;
}

uint32_t DatabaseManager::updateDatabase()
{
	Database* db = Database::getInstance();
	DBQuery query;

	int32_t databaseVersion = getDatabaseVersion();
	if(databaseVersion < 0)
		return 0;

	switch(databaseVersion)
	{
		case 0:
		{
			std::clog << "> Updating database to version 1 (Only db ready)" << std::endl;
			registerDatabaseConfig("db_version", 1);
			return 1;
		}

		case 1:
		{
			std::clog << "> Updating database to version 2 (Only db ready)" << std::endl;
			registerDatabaseConfig("db_version", 2);
			return 2;
		}

		case 2:
		{
			std::clog << "> Updating database to version 3 (bank balance)" << std::endl;
			db->executeQuery("ALTER TABLE `players` ADD `balance` BIGINT UNSIGNED NOT NULL DEFAULT 0");
			registerDatabaseConfig("db_version", 3);
			return 3;
		}

		default: break;
	}
	return 0;
}

bool DatabaseManager::getDatabaseConfig(std::string config, int32_t &value)
{
	Database* db = Database::getInstance();
	DBQuery query;
	query << "SELECT `value` FROM `server_config` WHERE `config` = " << db->escapeString(config) << ";";
	DBResult* result = db->storeQuery(query.str());
	if(!result)
		return false;

	value = atoi(result->getDataString("value").c_str());
	db->freeResult(result);
	return true;
}

bool DatabaseManager::getDatabaseConfig(std::string config, std::string &value)
{
	Database* db = Database::getInstance();
	DBQuery query;
	query << "SELECT `value` FROM `server_config` WHERE `config` = " << db->escapeString(config) << ";";
	DBResult* result = db->storeQuery(query.str());
	if(!result)
		return false;

	value = result->getDataString("value");
	db->freeResult(result);
	return true;
}

void DatabaseManager::registerDatabaseConfig(std::string config, int32_t value)
{
	Database* db = Database::getInstance();
	DBQuery query;

	int32_t tmp;
	if(!getDatabaseConfig(config, tmp))
		query << "INSERT INTO `server_config` VALUES (" << db->escapeString(config) << ", '" << value << "');";
	else
		query << "UPDATE `server_config` SET `value` = '" << value << "' WHERE `config` = " << db->escapeString(config) << ";";

	db->executeQuery(query.str());
}

void DatabaseManager::registerDatabaseConfig(std::string config, std::string value)
{
	Database* db = Database::getInstance();
	DBQuery query;

	std::string tmp;
	if(!getDatabaseConfig(config, tmp))
		query << "INSERT INTO `server_config` VALUES (" << db->escapeString(config) << ", " << db->escapeString(value) << ");";
	else
		query << "UPDATE `server_config` SET `value` = " << db->escapeString(value) << " WHERE `config` = " << db->escapeString(config) << ";";

	db->executeQuery(query.str());
}

void DatabaseManager::checkEncryption()
{
	int32_t currentValue = g_config.getNumber(ConfigManager::PASSWORD_TYPE);
	int32_t oldValue = 0;
	if(getDatabaseConfig("encryption", oldValue))
	{
		if(currentValue == oldValue)
			return;

		if(oldValue != PASSWORD_TYPE_PLAIN)
		{
			std::string oldName;
			if(oldValue == PASSWORD_TYPE_MD5)
				oldName = "md5";
			else if(oldValue == PASSWORD_TYPE_SHA1)
				oldName = "sha1";
			else
				oldName = "plain";

			g_config.setNumber(ConfigManager::PASSWORD_TYPE, oldValue);
			std::clog << "> WARNING: Unsupported password hashing switch! Change back passwordType in config.lua to \"" << oldName << "\"!" << std::endl;
			return;
		}

		switch(currentValue)
		{
			case PASSWORD_TYPE_MD5:
			{
				Database* db = Database::getInstance();
				DBQuery query;
				DBResult* result = db->storeQuery("SELECT `id`, `password`, `key` FROM `accounts`;");
				
				if(result)
				{
					do
					{
						query << "UPDATE `accounts` SET `password` = " << db->escapeString(transformToMD5(result->getDataString("password"))) << ", `key` = " << db->escapeString(transformToMD5(result->getDataString("key"))) << " WHERE `id` = " << result->getDataInt("id") << ";";
						db->executeQuery(query.str());
					}
					while(result->next());
						db->freeResult(result);
				}

				std::clog << "> Password type has been updated to MD5." << std::endl;
				break;
			}

			case PASSWORD_TYPE_SHA1:
			{
				Database* db = Database::getInstance();
				DBQuery query;
				DBResult* result = db->storeQuery("SELECT `id`, `password`, `key` FROM `accounts`;");
				
				if(result)
				{
					do
					{
						query << "UPDATE `accounts` SET `password` = " << db->escapeString(transformToSHA1(result->getDataString("password"))) << ", `key` = " << db->escapeString(transformToSHA1(result->getDataString("key"))) << " WHERE `id` = " << result->getDataInt("id") << ";";
						db->executeQuery(query.str());
					}
					while(result->next());
						db->freeResult(result);
				}

				std::clog << "> Password type has been updated to SHA1." << std::endl;
				break;
			}

			default: break;
		}
	}
	else if(g_config.getBool(ConfigManager::ACCOUNT_MANAGER))
	{
		switch(currentValue)
		{
			case PASSWORD_TYPE_MD5:
			{
				Database* db = Database::getInstance();
				DBQuery query;
				query << "UPDATE `accounts` SET `password` = " << db->escapeString(transformToMD5("1")) << " WHERE `id` = 1 AND `password` = '1';";
				db->executeQuery(query.str());
				break;
			}

			case PASSWORD_TYPE_SHA1:
			{
				Database* db = Database::getInstance();
				DBQuery query;
				query << "UPDATE `accounts` SET `password` = " << db->escapeString(transformToSHA1("1")) << " WHERE `id` = 1 AND `password` = '1';";
				db->executeQuery(query.str());
				break;
			}

			default: break;
		}
	}
	registerDatabaseConfig("encryption", currentValue);
}

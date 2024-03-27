//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// otserv main. The only place where things get instantiated.
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
#include <boost/asio.hpp>
#include "server.h"
#include <string>
#include <iostream>
#include <iomanip>
#include "otsystem.h"
#include "networkmessage.h"
#include "protocolgame.h"
#include <stdlib.h>
#include <time.h>
#include "game.h"
#include "iologindata.h"
#include "status.h"
#include "monsters.h"
#include "outfit.h"
#include "vocation.h"
#include "scriptmanager.h"
#include "configmanager.h"
#include "globalevent.h"
#include "tools.h"
#include "ban.h"
#include "rsa.h"
#include "spells.h"
#include "movement.h"
#include "talkaction.h"
#include "raids.h"
#include "quests.h"
#include "resources.h"
#include "group.h"
#include "databasemanager.h"

#ifdef __DEBUG_CRITICALSECTION__
OTSYS_THREAD_LOCK_CLASS::LogList OTSYS_THREAD_LOCK_CLASS::loglist;
#endif

#ifdef BOOST_NO_EXCEPTIONS
	#include <exception>
	void boost::throw_exception(std::exception const & e)
	{
		std::clog << "Boost exception: " << e.what() << std::endl;
	}
#endif

IPList serverIPs;

Ban g_bans;
Game g_game;
Npcs g_npcs;
ConfigManager g_config;
Monsters g_monsters;
Vocations g_vocations;

extern Actions* g_actions;
extern CreatureEvents* g_creatureEvents;
extern MoveEvents* g_moveEvents;
extern Spells* g_spells;
extern TalkActions* g_talkActions;
extern GlobalEvents* g_globalEvents;

RSA* g_otservRSA = NULL;
Server* g_server = NULL;

OTSYS_THREAD_SIGNALVAR g_loaderSignal;

#ifdef __EXCEPTION_TRACER__
#include "exception.h"
#endif
#include "networkmessage.h"

void startupErrorMessage(std::string errorStr)
{
	if(errorStr.length() > 0)
		std::clog << "ERROR: " << errorStr << std::endl;

	#ifdef WIN32
	system("pause");
	#else
	getchar();
	#endif
	exit(-1);
}

int main(int argc, char *argv[])
{
	#ifdef __EXCEPTION_TRACER__
	ExceptionHandler mainExceptionHandler;
	mainExceptionHandler.InstallHandler();
	#endif

	OutputHandler::getInstance();
	#ifdef WIN32
	SetConsoleTitle(STATUS_SERVER_NAME);
	#endif
	std::clog << STATUS_SERVER_NAME << " - Version " << STATUS_SERVER_VERSION << " (" << STATUS_SERVER_CODENAME << ")." << std::endl;
	std::clog << "Software Version: "  SOFTWARE_VERSION << "." << std::endl;
	std::clog << "Compiled with " << BOOST_COMPILER << std::endl;
	std::clog << "Compiled on " << __DATE__ << ' ' << __TIME__ << " for platform ";

#if defined(__amd64__) || defined(_M_X64)
	std::clog << "x64" << std::endl;
#elif defined(__i386__) || defined(_M_IX86) || defined(_X86_)
	std::clog << "x86" << std::endl;
#elif defined(__arm__)
	std::clog << "ARM" << std::endl;
#else
	std::clog << "unknown" << std::endl;
#endif
	std::clog << "A server developed by Talaturen, Kornholijo & Elf." << std::endl;
	std::clog << "Server modfied and updated by Tryller." << std::endl;
	std::clog << "Visit http://otland.net/." << std::endl;


	std::clog << std::endl;
	
#if !defined(WIN32) && !defined(__ROOT_PERMISSION__)
	if(getuid() == 0 || geteuid() == 0)
		std::clog << "WARNING: " << STATUS_SERVER_NAME << " has been executed as root user, it is recommended to execute is as a normal user." << std::endl;
		return 1;
	}
#endif

	// ignore sigpipe...
	#ifdef WIN32
	//nothing yet
	#else
	struct sigaction sigh;
	sigh.sa_handler = SIG_IGN;
	sigh.sa_flags = 0;
	sigemptyset(&sigh.sa_mask);
	sigaction(SIGPIPE, &sigh, NULL);
	#endif

	OTSYS_THREAD_SIGNALVARINIT(g_loaderSignal);

	//dispatcher thread
	g_game.setGameState(GAME_STATE_STARTUP);

	srand((unsigned int)OTSYS_TIME());
	// read global config
	std::clog << "Loading config" << std::endl;
	if(!g_config.load())
	{
		startupErrorMessage("Unable to load config.lua!");
		return -1;
	}

#ifdef WIN32
	const std::string& defaultPriority = g_config.getString(ConfigManager::DEFAULT_PRIORITY);
	if(strcasecmp(defaultPriority.c_str(), "high") == 0)
		SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	else if(strcasecmp(defaultPriority.c_str(), "above-normal") == 0)
		SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS);

	std::stringstream mutexName;
	mutexName << "forgottenserver_" << g_config.getNumber(ConfigManager::PORT);

	CreateMutex(NULL, FALSE, mutexName.str().c_str());
	if(GetLastError() == ERROR_ALREADY_EXISTS)
	{
		startupErrorMessage("Another instance of The Forgotten Server is already running with the same login port, please shut it down first or change ports for this one.");
		return -1;
	}
	
#endif

	//load RSA key
	std::clog << "Loading RSA key" << std::endl;
	const char* p("14299623962416399520070177382898895550795403345466153217470516082934737582776038882967213386204600674145392845853859217990626450972452084065728686565928113");
	const char* q("7630979195970404721891201847792002125535401292779123937207447574596692788513647179235335529307251350570728407373705564708871762033017096809910315212884101");
	const char* d("46730330223584118622160180015036832148732986808519344675210555262940258739805766860224610646919605860206328024326703361630109888417839241959507572247284807035235569619173792292786907845791904955103601652822519121908367187885509270025388641700821735345222087940578381210879116823013776808975766851829020659073");
	g_otservRSA = new RSA();
	g_otservRSA->setKey(p, q, d);

	std::clog << "Loading database driver..." << std::flush;
	Database* db = Database::getInstance();
	if(!db->isConnected())
	{
		switch(db->getDatabaseEngine())
		{
			case DATABASE_ENGINE_MYSQL:
				startupErrorMessage("Failed to connect to database, read doc/MYSQL_HELP for information or try SQLite which doesn't require any connection.");
				return -1;

			case DATABASE_ENGINE_SQLITE:
				startupErrorMessage("Failed to connect to sqlite database file, make sure it exists and is readable.");
				return -1;

			default:
				startupErrorMessage("Unkwown sqlType in config.lua, valid sqlTypes are: \"mysql\" and \"sqlite\".");
				return -1;
		}
	}
	std::clog << " " << db->getClientName() << " " << db->getClientVersion() << std::endl;

	std::clog << "Running database manager" << std::endl;
	DatabaseManager* dbManager = DatabaseManager::getInstance();
	if(!dbManager->isDatabaseSetup())
	{
		startupErrorMessage("The database you have specified in config.lua is empty, please import the schema to the database.");
		return -1;
	}

	for(uint32_t version = dbManager->updateDatabase(); version != 0; version = dbManager->updateDatabase())
		std::clog << "Database has been updated to version " << version << "." << std::endl;

	dbManager->checkEncryption();

	if(g_config.getBool(ConfigManager::OPTIMIZE_DATABASE) && !dbManager->optimizeTables())
		std::clog << "No tables were optimized." << std::endl;

	std::clog << "Loading bans" << std::endl;
	g_bans.init();

	std::clog << "Loading groups" << std::endl;
	if(!Groups::getInstance()->loadFromXml())
	{
		startupErrorMessage("Unable to load groups!");
		return -1;
	}

	std::clog << "Loading vocations" << std::endl;
	if(!g_vocations.loadFromXml())
	{
		startupErrorMessage("Unable to load vocations!");
		return -1;
	}

	std::clog << "Loading items (OTB)" << std::endl;
	if(Item::items.loadFromOtb("data/items/items.otb"))
	{
		startupErrorMessage("Unable to load items (OTB)!");
		return -1;
	}
	
	std::clog << "Loading items (XML)" << std::endl;
	if(!Item::items.loadFromXml())
	{
		startupErrorMessage("Unable to load items (XML)!");
		return -1;
	}
	
	std::clog << "Loading script systems" << std::endl;
	if(!ScriptingManager::getInstance()->loadScriptSystems())
		startupErrorMessage("");

	std::clog << "Loading monsters" << std::endl;
	if(!g_monsters.loadFromXml())
	{
		startupErrorMessage("Unable to load monsters!");
		return -1;
	}
	
	std::clog << "Loading outfits" << std::endl;
	Outfits* outfits = Outfits::getInstance();
	if(!outfits->loadFromXml())
	{
		startupErrorMessage("Unable to load outfits!");
		return -1;
	}

	std::clog << "Loading experience stages" << std::endl;
	if(!g_game.loadExperienceStages())
	{
		startupErrorMessage("Unable to load experience stages!");
		return -1;
	}

	std::string passwordType = asLowerCaseString(g_config.getString(ConfigManager::PASSWORDTYPE));
	if(passwordType == "md5")
	{
		g_config.setNumber(ConfigManager::PASSWORD_TYPE, PASSWORD_TYPE_MD5);
		std::clog << "Using MD5 passwords" << std::endl;
	}
	else if(passwordType == "sha1")
	{
		g_config.setNumber(ConfigManager::PASSWORD_TYPE, PASSWORD_TYPE_SHA1);
		std::clog << "Using SHA1 passwords" << std::endl;
	}
	else
	{
		g_config.setNumber(ConfigManager::PASSWORD_TYPE, PASSWORD_TYPE_PLAIN);
		std::clog << "Using plaintext passwords" << std::endl;
	}

	std::clog << "Checking world type... ";
	std::string worldType = asLowerCaseString(g_config.getString(ConfigManager::WORLD_TYPE));
	if(worldType == "pvp" || worldType == "open" || worldType == "open-pvp")
	{
		g_game.setWorldType(WORLD_TYPE_PVP);
		std::clog << "PvP" << std::endl;
	}
	else if(worldType == "no-pvp" || worldType == "nopvp" || worldType == "safe")
	{
		g_game.setWorldType(WORLD_TYPE_NO_PVP);
		std::clog << "Non PvP" << std::endl;
	}
	else if(worldType == "pvp-enforced" || worldType == "enforced" || worldType == "war")
	{
		g_game.setWorldType(WORLD_TYPE_PVP_ENFORCED);
		std::clog << "PvP Enforced" << std::endl;
	}
	else
	{
		std::clog << std::endl;
		startupErrorMessage("Unknown world type: " + g_config.getString(ConfigManager::WORLD_TYPE));
		return -1;
	}

	Status* status = Status::getInstance();
	status->setMaxPlayersOnline(g_config.getNumber(ConfigManager::MAX_PLAYERS));
	status->setMapAuthor(g_config.getString(ConfigManager::MAP_AUTHOR));
	status->setMapName(g_config.getString(ConfigManager::MAP_NAME));

	std::clog << "Loading map" << std::endl;
	if(!g_game.loadMap(g_config.getString(ConfigManager::MAP_NAME)))
	{
		startupErrorMessage("Failed to load map");
		return -1;
	}


	g_game.setGameState(GAME_STATE_INIT);
	g_game.timedHighscoreUpdate();

	int32_t autoSaveEachMinutes = g_config.getNumber(ConfigManager::AUTO_SAVE_EACH_MINUTES);
	if(autoSaveEachMinutes > 0)
		Scheduler::getScheduler().addEvent(createSchedulerTask(autoSaveEachMinutes * 1000 * 60, boost::bind(&Game::autoSave, &g_game)));

	if(g_config.getBool(ConfigManager::SERVERSAVE_ENABLED))
	{
		int32_t serverSaveHour = g_config.getNumber(ConfigManager::SERVERSAVE_H);
		if(serverSaveHour >= 0 && serverSaveHour <= 24)
		{
			time_t timeNow = time(NULL);
			tm* timeinfo = localtime(&timeNow);

			if(serverSaveHour == 0)
				serverSaveHour = 23;
			else
				serverSaveHour--;

			timeinfo->tm_hour = serverSaveHour;
			timeinfo->tm_min = 55;
			timeinfo->tm_sec = 0;
			time_t difference = (time_t)difftime(mktime(timeinfo), timeNow);
			if(difference < 0)
				difference += 86400;

			Scheduler::getScheduler().addEvent(createSchedulerTask(difference * 1000, boost::bind(&Game::prepareServerSave, &g_game)));
		}
	}

	std::clog << "Loaded all modules, server starting up..." << std::endl;

	std::pair<uint32_t, uint32_t> IpNetMask;
	IpNetMask.first  = inet_addr("127.0.0.1");
	IpNetMask.second = 0xFFFFFFFF;
	serverIPs.push_back(IpNetMask);

	char szHostName[128];
	if(gethostname(szHostName, 128) == 0)
	{
		hostent *he = gethostbyname(szHostName);
		if(he)
		{
			unsigned char** addr = (unsigned char**)he->h_addr_list;
			while(addr[0] != NULL)
			{
				IpNetMask.first  = *(uint32_t*)(*addr);
				IpNetMask.second = 0x0000FFFF;
				serverIPs.push_back(IpNetMask);
				addr++;
			}
		}
	}
	std::string ip;
	ip = g_config.getString(ConfigManager::IP);

	uint32_t resolvedIp = inet_addr(ip.c_str());
	if(resolvedIp == INADDR_NONE)
	{
		struct hostent* he = gethostbyname(ip.c_str());
		if(he != 0)
			resolvedIp = *(uint32_t*)he->h_addr;
		else
		{
			std::clog << "ERROR: Cannot resolve " << ip << "!" << std::endl;
			startupErrorMessage("");
		}
	}

	IpNetMask.first  = resolvedIp;
	IpNetMask.second = 0;
	serverIPs.push_back(IpNetMask);

	Server server(INADDR_ANY, g_config.getNumber(ConfigManager::PORT));
	std::clog << g_config.getString(ConfigManager::SERVER_NAME) << " Server Online!" << std::endl << std::endl;
	g_server = &server;
	server.run();

	IOLoginData::getInstance()->resetOnlineStatus();
	g_game.setGameState(GAME_STATE_NORMAL);
	OTSYS_THREAD_SIGNAL_SEND(g_loaderSignal);

	return 0;
}

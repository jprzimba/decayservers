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
#include "otsystem.h"

#include <iostream>
#include <fstream>
#include <iomanip>
#ifndef WINDOWS
#include <unistd.h>
#endif
#include <boost/config.hpp>

#include "server.h"
#include "networkmessage.h"

#include "game.h"
#include "chat.h"
#include "tools.h"
#include "rsa.h"
#include "textlogger.h"

#include "protocollogin.h"
#include "protocolgame.h"
#include "status.h"

#include "configmanager.h"
#include "scriptmanager.h"
#include "databasemanager.h"

#include "iologindata.h"
#include "ioban.h"

#include "outfit.h"
#include "vocation.h"
#include "group.h"

#include "monsters.h"
#ifdef __OTSERV_ALLOCATOR__
#include "allocator.h"
#endif
#ifdef __EXCEPTION_TRACER__
#include "exception.h"
#endif

#include "resources.h"

#ifdef __NO_BOOST_EXCEPTIONS__
#include <exception>

void boost::throw_exception(std::exception const & e)
{
	std::clog << "Boost exception: " << e.what() << std::endl;
}
#endif

ConfigManager g_config;
Game g_game;
Monsters g_monsters;
Npcs g_npcs;
Chat g_chat;

RSA* g_otservRSA = NULL;
Server* g_server = NULL;

IpList serverIps;
boost::mutex g_loaderLock;
boost::condition_variable g_loaderSignal;

boost::unique_lock<boost::mutex> g_loaderUniqueLock(g_loaderLock);

#ifndef WINDOWS
int32_t OTSYS_getch()
{
	struct termios oldt;
	tcgetattr(STDIN_FILENO, &oldt);

	struct termios newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);

	int32_t ch = getchar();
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	return ch;
}

void signalHandler(int32_t sig)
{
	uint32_t tmp = 0;
	switch(sig)
	{
		case SIGHUP:
			Dispatcher::getInstance().addTask(createTask(
				boost::bind(&Game::saveGameState, &g_game, (uint8_t)SAVE_PLAYERS | (uint8_t)SAVE_MAP | (uint8_t)SAVE_STATE)));
			break;

		case SIGTRAP:
			g_game.cleanMap(tmp);
			break;

		case SIGCHLD:
			g_game.proceduralRefresh();
			break;

		case SIGUSR1:
			Dispatcher::getInstance().addTask(createTask(
				boost::bind(&Game::setGameState, &g_game, GAMESTATE_CLOSED)));
			break;

		case SIGUSR2:
			g_game.setGameState(GAMESTATE_NORMAL);
			break;

		case SIGCONT:
			g_game.reloadInfo(RELOAD_ALL);
			break;

		case SIGQUIT:
			Dispatcher::getInstance().addTask(createTask(
				boost::bind(&Game::setGameState, &g_game, GAMESTATE_SHUTDOWN)));
			break;

		case SIGTERM:
			Dispatcher::getInstance().addTask(createTask(
				boost::bind(&Game::shutdown, &g_game)));
			break;

		default:
			break;
	}
}

void runfileHandler(void)
{
	std::ofstream runfile(g_config.getString(ConfigManager::RUNFILE).c_str(), std::ios::trunc | std::ios::out);
	runfile.close();
}
#else
int32_t OTSYS_getch()
{
	return (int32_t)getchar();
}
#endif

void allocationHandler()
{
	puts("Allocation failed, server out of memory!\nDecrease size of your map or compile in a 64-bit mode.");
	OTSYS_getch();
	std::exit(-1);
}

void startupErrorMessage(std::string error = "")
{
	// we will get a crash here as the threads aren't going down smoothly
	if(error.length() > 0)
		std::clog << std::endl << ">> ERROR: " << error << std::endl;

	OTSYS_getch();
	std::exit(-1);
}

void mainLoader(int argc, char *argv[]);
int main(int argc, char *argv[])
{
	std::set_new_handler(allocationHandler);
	g_config.startup();

	#ifdef __OTSERV_ALLOCATOR_STATS__
	boost::thread(boost::bind(&allocatorStatsThread, (void*)NULL));
	// TODO: destruct this thread...
	#endif
	#ifdef __EXCEPTION_TRACER__
	ExceptionHandler mainExceptionHandler;
	mainExceptionHandler.InstallHandler();
	#endif
	#ifndef WINDOWS

	// ignore sigpipe...
	struct sigaction sigh;
	sigh.sa_handler = SIG_IGN;
	sigh.sa_flags = 0;
	sigemptyset(&sigh.sa_mask);
	sigaction(SIGPIPE, &sigh, NULL);

	// register signals
	signal(SIGHUP, signalHandler); //save
	signal(SIGTRAP, signalHandler); //clean
	signal(SIGCHLD, signalHandler); //refresh
	signal(SIGUSR1, signalHandler); //close server
	signal(SIGUSR2, signalHandler); //open server
	signal(SIGCONT, signalHandler); //reload all
	signal(SIGQUIT, signalHandler); //save & shutdown
	signal(SIGTERM, signalHandler); //shutdown
	#endif

	OutputHandler::getInstance();
	Dispatcher::getInstance().addTask(createTask(boost::bind(mainLoader, argc, argv)));

	g_loaderSignal.wait(g_loaderUniqueLock);


	std::string outPath = g_config.getString(ConfigManager::OUTPUT_LOG), errPath = g_config.getString(ConfigManager::ERROR_LOG);
	if(outPath.length() < 3)
		outPath = "";
	else if(outPath[0] != '/' && outPath[1] != ':')
	{
		outPath = getFilePath(FILE_TYPE_LOG, outPath);
		std::clog << ">> Logging output to file: " << outPath << std::endl;
	}

	if(errPath.length() < 3)
		errPath = "";
	else if(errPath[0] != '/' && errPath[1] != ':')
	{
		errPath = getFilePath(FILE_TYPE_LOG, errPath);
		std::clog << ">> Logging errors to file: " << errPath << std::endl;
	}

	if(outPath != "")
	{
		boost::shared_ptr<std::ofstream> outFile;
		outFile.reset(new std::ofstream(outPath.c_str(), (g_config.getBool(ConfigManager::TRUNCATE_LOGS) ?
			std::ios::trunc : std::ios::app) | std::ios::out));
		if(!outFile->is_open())
			startupErrorMessage("Could not open output log file for writing!");

		std::clog.rdbuf(outFile->rdbuf());
	}

	if(errPath != "")
	{
		boost::shared_ptr<std::ofstream> errFile;
		errFile.reset(new std::ofstream(errPath.c_str(), (g_config.getBool(ConfigManager::TRUNCATE_LOGS) ?
			std::ios::trunc : std::ios::app) | std::ios::out));
		if(!errFile->is_open())
			startupErrorMessage("Could not open error log file for writing!");

		std::cerr.rdbuf(errFile->rdbuf());
	}

	Server server(INADDR_ANY, g_config.getNumber(ConfigManager::LOGIN_PORT));
	std::clog << ">> " << g_config.getString(ConfigManager::SERVER_NAME) << " Server Online!" << std::endl << std::endl;
	g_server = &server;
	server.run();

#ifdef __EXCEPTION_TRACER__
	mainExceptionHandler.RemoveHandler();
#endif

	return 0;
}

void mainLoader(int argc, char *argv[])
{
	//dispatcher thread
	srand((uint32_t)OTSYS_TIME());
	#if defined(WINDOWS)
	SetConsoleTitle(SOFTWARE_NAME);
	#endif

	g_game.setGameState(GAMESTATE_STARTUP);
	#if !defined(WINDOWS) && !defined(__ROOT_PERMISSION__)
	if(!getuid() || !geteuid())
	{
		std::clog << ">> WARNING: " << SOFTWARE_NAME << " has been executed as super user! It is "
			<< "recommended to run as a normal user." << std::endl << "Continue? (y/N)" << std::endl;
		char buffer = OTSYS_getch();
		if(buffer != 121 && buffer != 89)
			startupErrorMessage("Aborted.");
	}
	#endif

	std::clog << ">> " << SOFTWARE_NAME << ", version " << SOFTWARE_VERSION << "." << std::endl;
	std::clog << ">> Compiled with " << BOOST_COMPILER << " at " << __DATE__ << ", " << __TIME__ << "." << std::endl;
	std::clog << ">> A server developed by " << SOFTWARE_DEVELOPERS << "." << std::endl;
	std::clog << ">> Visit our GitHub: http://github.com/tryller/otserv." << std::endl << std::endl;

	std::clog << ">> Loading config (" << g_config.getString(ConfigManager::CONFIG_FILE) << ")" << std::endl;
	if(!g_config.load())
		startupErrorMessage("Unable to load " + g_config.getString(ConfigManager::CONFIG_FILE) + "!");

#ifndef WINDOWS
	if(g_config.getBool(ConfigManager::DAEMONIZE))
	{
		std::clog << "> Daemonization... ";
		if(fork())
		{
			std::clog << "succeed, bye!" << std::endl;
			exit(0);
		}
		else
			std::clog << "failed, continuing." << std::endl;
	}

#endif
	// silently append trailing slash
	std::string path = g_config.getString(ConfigManager::DATA_DIRECTORY);
	g_config.setString(ConfigManager::DATA_DIRECTORY, path.erase(path.find_last_not_of("/") + 1) + "/");

	path = g_config.getString(ConfigManager::LOGS_DIRECTORY);
	g_config.setString(ConfigManager::LOGS_DIRECTORY, path.erase(path.find_last_not_of("/") + 1) + "/");

	std::clog << ">> Opening logs" << std::endl;
	Logger::getInstance()->open();

	IntegerVec cores = vectorAtoi(explodeString(g_config.getString(ConfigManager::CORES_USED), ","));
	if(cores[0] != -1)
	{
	#ifdef WINDOWS
		int32_t mask = 0;
		for(IntegerVec::iterator it = cores.begin(); it != cores.end(); ++it)
			mask += 1 << (*it);

		SetProcessAffinityMask(GetCurrentProcess(), mask);
	}

	std::stringstream mutexName;
	mutexName << "forgottenserver_" << g_config.getNumber(ConfigManager::WORLD_ID);

	CreateMutex(NULL, FALSE, mutexName.str().c_str());
	if(GetLastError() == ERROR_ALREADY_EXISTS)
		startupErrorMessage("Another instance of The Forgotten Server is already running with the same worldId.\nIf you want to run multiple servers, please change the worldId in configuration file.");

	std::string defaultPriority = asLowerCaseString(g_config.getString(ConfigManager::DEFAULT_PRIORITY));
	if(defaultPriority == "realtime")
		SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
	else if(defaultPriority == "high")
		SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	else if(defaultPriority == "higher")
		SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS);

	#else
		cpu_set_t mask;
		CPU_ZERO(&mask);
		for(IntegerVec::iterator it = cores.begin(); it != cores.end(); ++it)
			CPU_SET((*it), &mask);

		sched_setaffinity(getpid(), (int32_t)sizeof(mask), &mask);
	}

	std::string runPath = g_config.getString(ConfigManager::RUNFILE);
	if(runPath != "" && runPath.length() > 2)
	{
		std::ofstream runFile(runPath.c_str(), std::ios::trunc | std::ios::out);
		runFile << getpid();
		runFile.close();
		atexit(runfileHandler);
	}

	if(!nice(g_config.getNumber(ConfigManager::NICE_LEVEL))) {}
	#endif

	std::string passwordType = asLowerCaseString(g_config.getString(ConfigManager::PASSWORD_TYPE));
	if(passwordType == "md5")
	{
		g_config.setNumber(ConfigManager::PASSWORDTYPE, PASSWORD_TYPE_MD5);
		std::clog << ">> Using MD5 passwords" << std::endl;
	}
	else if(passwordType == "sha1")
	{
		g_config.setNumber(ConfigManager::PASSWORDTYPE, PASSWORD_TYPE_SHA1);
		std::clog << ">> Using SHA1 passwords" << std::endl;
	}
	else
	{
		g_config.setNumber(ConfigManager::PASSWORD_TYPE, PASSWORD_TYPE_PLAIN);
		std::clog << ">> Using PLAIN-TEXT passwords" << std::endl;
	}

	std::clog << ">> Loading RSA key" << std::endl;
	const char* p("14299623962416399520070177382898895550795403345466153217470516082934737582776038882967213386204600674145392845853859217990626450972452084065728686565928113");
	const char* q("7630979195970404721891201847792002125535401292779123937207447574596692788513647179235335529307251350570728407373705564708871762033017096809910315212884101");
	const char* d("46730330223584118622160180015036832148732986808519344675210555262940258739805766860224610646919605860206328024326703361630109888417839241959507572247284807035235569619173792292786907845791904955103601652822519121908367187885509270025388641700821735345222087940578381210879116823013776808975766851829020659073");
	g_otservRSA = new RSA();
	g_otservRSA->setKey(p, q, d);

	std::clog << ">> Starting SQL connection" << std::endl;
	Database* db = Database::getInstance();
	if(db && db->isConnected())
	{
		std::clog << ">> Running Database Manager" << std::endl;
		if(!DatabaseManager::getInstance()->isDatabaseSetup())
			startupErrorMessage("The database you specified in config.lua is empty, please import schemas/<dbengine>.sql to the database (if you are using MySQL, please read doc/MYSQL_HELP for more information).");
		else
		{
			uint32_t version = 0;
			do
			{
				version = DatabaseManager::getInstance()->updateDatabase();
				if(version == 0)
					break;

				std::clog << ">> Database has been updated to version: " << version << "." << std::endl;
			}
			while(version < VERSION_DATABASE);
		}

		DatabaseManager::getInstance()->checkTriggers();
		DatabaseManager::getInstance()->checkEncryption();
		if(g_config.getBool(ConfigManager::OPTIMIZE_DATABASE) && !DatabaseManager::getInstance()->optimizeTables())
			std::clog << ">> No tables were optimized." << std::endl;
	}
	else
		startupErrorMessage("Couldn't estabilish connection to SQL database!");

	std::clog << ">> Loading items (OTB)" << std::endl;
	if(Item::items.loadFromOtb(getFilePath(FILE_TYPE_OTHER, "items/items.otb")))
		startupErrorMessage("Unable to load items (OTB)!");

	std::clog << ">> Loading items (XML)" << std::endl;
	if(!Item::items.loadFromXml())
	{
		std::clog << "Unable to load items (XML)! Continue? (y/N)" << std::endl;
		char buffer = OTSYS_getch();
		if(buffer != 121 && buffer != 89)
			startupErrorMessage("Unable to load items (XML)!");
	}

	std::clog << ">> Loading groups" << std::endl;
	if(!Groups::getInstance()->loadFromXml())
		startupErrorMessage("Unable to load groups!");

	std::clog << ">> Loading vocations" << std::endl;
	if(!Vocations::getInstance()->loadFromXml())
		startupErrorMessage("Unable to load vocations!");

	std::clog << ">> Loading script systems" << std::endl;
	if(!ScriptManager::getInstance()->loadSystem())
		startupErrorMessage();

	std::clog << ">> Loading chat channels" << std::endl;
	if(!g_chat.loadFromXml())
		startupErrorMessage("Unable to load chat channels!");

	std::clog << ">> Loading outfits" << std::endl;
	if(!Outfits::getInstance()->loadFromXml())
		startupErrorMessage("Unable to load outfits!");

	std::clog << ">> Loading experience stages" << std::endl;
	if(!g_game.loadExperienceStages())
		startupErrorMessage("Unable to load experience stages!");

	std::clog << ">> Loading monsters" << std::endl;
	if(!g_monsters.loadFromXml())
	{
		std::clog << "Unable to load monsters! Continue? (y/N)" << std::endl;
		char buffer = OTSYS_getch();
		if(buffer != 121 && buffer != 89)
			startupErrorMessage("Unable to load monsters!");
	}

	if(fileExists(getFilePath(FILE_TYPE_OTHER, "npc/npcs.xml").c_str()))
	{
		std::clog << ">> Loading npcs" << std::endl;
		if(!g_npcs.loadFromXml())
		{
			std::clog << "Unable to load npcs! Continue? (y/N)" << std::endl;
			char buffer = OTSYS_getch();
			if(buffer != 121 && buffer != 89)
				startupErrorMessage("Unable to load npcs!");
		}
	}

	std::clog << ">> Loading raids" << std::endl;
	Raids::getInstance()->loadFromXml();

	std::clog << ">> Loading mods..." << std::endl;
	if(!ScriptManager::getInstance()->loadMods())
		startupErrorMessage("Unable to load mods!");

	std::clog << ">> Loading map and spawns..." << std::endl;
	if(!g_game.loadMap(g_config.getString(ConfigManager::MAP_NAME)))
		startupErrorMessage();

	std::clog << ">> Checking world type... ";
	std::string worldType = asLowerCaseString(g_config.getString(ConfigManager::WORLD_TYPE));
	if(worldType == "pvp" || worldType == "2" || worldType == "normal")
	{
		g_game.setWorldType(WORLDTYPE_PVP);
		std::clog << "PvP" << std::endl;
	}
	else if(worldType == "no-pvp" || worldType == "nopvp" || worldType == "non-pvp" || worldType == "nonpvp" || worldType == "1" || worldType == "safe")
	{
		g_game.setWorldType(WORLDTYPE_NOPVP);
		std::clog << "NoN-PvP" << std::endl;
	}
	else if(worldType == "pvp-enforced" || worldType == "pvpenforced" || worldType == "pvp-enfo" || worldType == "pvpenfo" || worldType == "pvpe" || worldType == "enforced" || worldType == "enfo" || worldType == "3" || worldType == "war")
	{
		g_game.setWorldType(WORLDTYPE_ENFORCED);
		std::clog << "PvP-Enforced" << std::endl;
	}
	else
	{
		std::clog << std::endl;
		startupErrorMessage("Unknown world type: " + g_config.getString(ConfigManager::WORLD_TYPE));
	}

	std::clog << ">> Initializing game state modules and registering services..." << std::endl;
	g_game.setGameState(GAMESTATE_INIT);

	std::pair<uint32_t, uint32_t> IpNetMask;

	IpNetMask.first  = inet_addr("127.0.0.1");
	IpNetMask.second = 0xFFFFFFFF;
	serverIps.push_back(IpNetMask);

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
				serverIps.push_back(IpNetMask);
				addr++;
			}
		}
	}

	std::string ip = g_config.getString(ConfigManager::IP);

	uint32_t resolvedIp = inet_addr(ip.c_str());
	if(resolvedIp == INADDR_NONE)
	{
		struct hostent* he = gethostbyname(ip.c_str());
		if(he != 0)
			resolvedIp = *(uint32_t*)he->h_addr;
		else
		{
			std::clog << "ERROR: Cannot resolve " << ip << "!" << std::endl;
			#ifdef WIN32
			getchar();
			#endif
			exit(-1);
		}
	}

	IpNetMask.first  = resolvedIp;
	IpNetMask.second = 0;
	serverIps.push_back(IpNetMask);

	#if !defined(WIN32) && !defined(__ROOT_PERMISSION__)
	if(getuid() == 0 || geteuid() == 0)
		std::clog << ">> WARNING: " << SOFTWARE_NAME << " has been executed as root user, it is recommended to execute is as a normal user." << std::endl;
	#endif
	g_game.setGameState(GAMESTATE_NORMAL);

	g_game.start();
	g_loaderSignal.notify_all();
}

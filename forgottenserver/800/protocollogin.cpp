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
#include "resources.h"
#include <iomanip>

#include "protocollogin.h"
#include "tools.h"
#include "rsa.h"

#include "iologindata.h"
#include "ioban.h"

#include "outputmessage.h"
#include "connection.h"

#include "configmanager.h"
#include "game.h"

extern ConfigManager g_config;
extern Game g_game;
extern RSA* g_otservRSA;
extern IpList serverIps;

#ifdef __ENABLE_SERVER_DIAGNOSTIC__
uint32_t ProtocolLogin::protocolLoginCount = 0;
#endif

void ProtocolLogin::deleteProtocolTask()
{
#ifdef __DEBUG_NET_DETAIL__
	std::cout << "Deleting ProtocolLogin" << std::endl;
#endif
	Protocol::deleteProtocolTask();
}

void ProtocolLogin::disconnectClient(uint8_t error, const char* message)
{
	OutputMessage_ptr output = OutputMessagePool::getInstance()->getOutputMessage(this, false);
	if(output)
	{
		TRACK_MESSAGE(output);
		output->AddByte(error);
		output->AddString(message);
		OutputMessagePool::getInstance()->send(output);
	}

	getConnection()->closeConnection();
}

bool ProtocolLogin::parseFirstPacket(NetworkMessage& msg)
{
	if(g_game.getGameState() == GAME_STATE_SHUTDOWN)
	{
		getConnection()->closeConnection();
		return false;
	}

	uint32_t clientIp = getConnection()->getIP();
	/*uint16_t operatingSystem =*/ msg.GetU16();
	uint16_t version = msg.GetU16();

	msg.SkipBytes(12);

	if(version <= 760)
		disconnectClient(0x0A, CLIENT_VERSION_STRING);

	if(!RSA_decrypt(g_otservRSA, msg))
	{
		getConnection()->closeConnection();
		return false;
	}

	uint32_t key[4];
	key[0] = msg.GetU32();
	key[1] = msg.GetU32();
	key[2] = msg.GetU32();
	key[3] = msg.GetU32();
	enableXTEAEncryption();
	setXTEAKey(key);

	uint32_t accnumber = msg.GetU32();
	std::string password = msg.GetString();

	if(!accnumber)
	{
		if(!g_config.getBool(ConfigManager::ACCOUNT_MANAGER))
		{
			disconnectClient(0x0A, "Invalid account name.");
			return false;
		}

		accnumber = 1;
		password = "1";
	}

	if(version < CLIENT_VERSION_MIN || version > CLIENT_VERSION_MAX)
	{
		disconnectClient(0x0A, CLIENT_VERSION_STRING);
		return false;
	}

	if(g_game.getGameState() < GAME_STATE_NORMAL)
	{
		disconnectClient(0x0A, "Server is just starting up, please wait.");
		return false;
	}

	if(g_game.getGameState() == GAME_STATE_MAINTAIN)
	{
		disconnectClient(0x0A, "Server is under maintenance, please re-connect in a while.");
		return false;
	}

	if(ConnectionManager::getInstance()->isDisabled(clientIp))
	{
		disconnectClient(0x0A, "Too many connections attempts from your IP address, please try again later.");
		return false;
	}

	if(IOBan::getInstance()->isIpBanished(clientIp))
	{
		disconnectClient(0x0A, "Your IP is banished!");
		return false;
	}

	uint32_t serverIp = serverIps[0].first;
	for(IpList::iterator it = serverIps.begin(); it != serverIps.end(); ++it)
	{
		if((it->first & it->second) != (clientIp & it->second))
			continue;

		serverIp = it->first;
		break;
	}

	Account account = IOLoginData::getInstance()->loadAccount(accnumber);
	if(!(accnumber != 0 && account.accnumber == accnumber &&
		passwordTest(password, account.password)))
	{
		ConnectionManager::getInstance()->addLoginAttempt(clientIp, false);
		disconnectClient(0x0A, "Account number or password is not correct.");
		return false;
	}

	Ban ban;
	ban.value = account.accnumber;

	ban.type = BAN_ACCOUNT;
	if(IOBan::getInstance()->getData(ban) && !IOLoginData::getInstance()->hasFlag(account.accnumber, PlayerFlag_CannotBeBanned))
	{
		bool deletion = ban.expires < 0;
		std::string name_ = "Automatic ";
		if(!ban.adminId)
			name_ += (deletion ? "deletion" : "banishment");
		else
			IOLoginData::getInstance()->getNameByGuid(ban.adminId, name_, true);

		char buffer[500 + ban.comment.length()];
		sprintf(buffer, "Your account has been %s at:\n%s by: %s,\nfor the following reason:\n%s.\nThe action taken was:\n%s.\nThe comment given was:\n%s.\nYour %s%s.",
			(deletion ? "deleted" : "banished"), formatDateShort(ban.added).c_str(), name_.c_str(),
			getReason(ban.reason).c_str(), getAction(ban.action, false).c_str(), ban.comment.c_str(),
			(deletion ? "account won't be undeleted" : "banishment will be lifted at:\n"),
			(deletion ? "." : formatDateShort(ban.expires, true).c_str()));

		disconnectClient(0x0A, buffer);
		return false;
	}

	//Remove premium days
	IOLoginData::getInstance()->removePremium(account);
	if(!g_config.getBool(ConfigManager::ACCOUNT_MANAGER) && !account.charList.size())
	{
		disconnectClient(0x0A, std::string("This account does not contain any character yet.\nCreate a new character on the "
			+ g_config.getString(ConfigManager::SERVER_NAME) + " website at " + g_config.getString(ConfigManager::URL) + ".").c_str());
		return false;
	}

	ConnectionManager::getInstance()->addLoginAttempt(clientIp, true);
	if(OutputMessage_ptr output = OutputMessagePool::getInstance()->getOutputMessage(this, false))
	{
		TRACK_MESSAGE(output);
		output->AddByte(0x14);

		char motd[750];
		sprintf(motd, "%d\n%s", g_game.getMotdId(), g_config.getString(ConfigManager::MOTD).c_str());
		output->AddString(motd);

		//Add char list
		output->AddByte(0x64);
		if(g_config.getBool(ConfigManager::ACCOUNT_MANAGER) && accnumber != 1)
		{
			output->AddByte((uint8_t)account.charList.size() + 1);
			output->AddString("Account Manager");
			output->AddString(g_config.getString(ConfigManager::SERVER_NAME));
			output->AddU32(serverIp);
			output->AddU16(g_config.getNumber(ConfigManager::LOGIN_PORT));
		}
		else
			output->AddByte((uint8_t)account.charList.size());

		for(Characters::iterator it = account.charList.begin(); it != account.charList.end(); it++)
		{
			output->AddString((*it));
			if(g_config.getBool(ConfigManager::ON_OR_OFF_CHARLIST))
			{
				if(g_game.getPlayerByName((*it)))
					output->AddString("Online");
				else
					output->AddString("Offline");
			}
			else
				output->AddString(g_config.getString(ConfigManager::SERVER_NAME));

			output->AddU32(serverIp);
			output->AddU16(g_config.getNumber(ConfigManager::LOGIN_PORT));
		}

		//Add premium days
		if(g_config.getBool(ConfigManager::FREE_PREMIUM))
			output->AddU16(65535); //client displays free premium
		else
			output->AddU16(account.premiumDays);

		OutputMessagePool::getInstance()->send(output);
	}

	getConnection()->closeConnection();
	return true;
}

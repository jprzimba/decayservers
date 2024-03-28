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
#include <iomanip>

#include "tools.h"
#include "md5.h"
#include "sha1.h"

#include "vocation.h"
#include "configmanager.h"
extern ConfigManager g_config;

std::string transformToSHA1(std::string plainText, bool upperCase /*= false*/)
{
	SHA1 sha1;
	unsigned sha1Hash[5];
	std::stringstream hexStream;

	sha1.Input((const uint8_t*)plainText.c_str(), plainText.length());
	sha1.Result(sha1Hash);

	hexStream.flags(std::ios::hex | std::ios::uppercase);
	for(uint32_t i = 0; i < 5; ++i)
		hexStream << std::setw(8) << std::setfill('0') << (uint32_t)sha1Hash[i];

	std::string hexStr = hexStream.str();
	if(!upperCase)
		toLowerCaseString(hexStr);

	return hexStr;
}

std::string transformToMD5(std::string plainText, bool upperCase /*= false*/)
{
	MD5_CTX m_md5;
	std::stringstream hexStream;

	MD5Init(&m_md5, 0);
	MD5Update(&m_md5, (const uint8_t*)plainText.c_str(), plainText.length());
	MD5Final(&m_md5);

	hexStream.flags(std::ios::hex | std::ios::uppercase);
	for(uint32_t i = 0; i < 16; ++i)
		hexStream << std::setw(2) << std::setfill('0') << (uint32_t)m_md5.digest[i];

	std::string hexStr = hexStream.str();
	if(!upperCase)
		toLowerCaseString(hexStr);

	return hexStr;
}

bool passwordTest(const std::string &plain, std::string &hash)
{
	switch(g_config.getNumber(ConfigManager::PASSWORDTYPE))
	{
		case PASSWORD_TYPE_MD5:
		{
			std::transform(hash.begin(), hash.end(), hash.begin(), upchar);
			return transformToMD5(plain, true) == hash;
			break;
		}

		case PASSWORD_TYPE_SHA1:
		{
			std::transform(hash.begin(), hash.end(), hash.begin(), upchar);
			return transformToSHA1(plain, true) == hash;
			break;
		}

		default:
		{
			return plain == hash;
			break;
		}
	}
	return false;
}

void replaceString(std::string& text, const std::string key, const std::string value)
{
	if(value.find(key) != std::string::npos) //don't allow infinite loops
		return;

	for(std::string::size_type keyStart = text.find(key); keyStart
		!= std::string::npos; keyStart = text.find(key))
		text.replace(keyStart, key.size(), value);
}

void trim_right(std::string& source, const std::string& t)
{
	source.erase(source.find_last_not_of(t)+1);
}

void trim_left(std::string& source, const std::string& t)
{
	source.erase(0, source.find_first_not_of(t));
}

void toLowerCaseString(std::string& source)
{
	std::transform(source.begin(), source.end(), source.begin(), tolower);
}

void toUpperCaseString(std::string& source)
{
	std::transform(source.begin(), source.end(), source.begin(), upchar);
}

std::string asLowerCaseString(const std::string& source)
{
	std::string s = source;
	toLowerCaseString(s);
	return s;
}

std::string asUpperCaseString(const std::string& source)
{
	std::string s = source;
	toUpperCaseString(s);
	return s;
}

bool booleanString(std::string source)
{
	toLowerCaseString(source);
	return (source == "yes" || source == "true" || atoi(source.c_str()) > 0);
}

bool readXMLInteger(xmlNodePtr node, const char* tag, int& value)
{
	char* nodeValue = (char*)xmlGetProp(node, (xmlChar*)tag);
	if(!nodeValue)
		return false;

	value = atoi(nodeValue);
	xmlFree(nodeValue);
	return true;
}

#if defined WINDOWS && !defined __GNUC__
bool readXMLInteger(xmlNodePtr node, const char* tag, int32_t& value)
{
	char* nodeValue = (char*)xmlGetProp(node, (xmlChar*)tag);
	if(!nodeValue)
		return false;

	value = atoi(nodeValue);
	xmlFree(nodeValue);
	return true;
}
#endif

bool readXMLInteger64(xmlNodePtr node, const char* tag, int64_t& value)
{
	char* nodeValue = (char*)xmlGetProp(node, (xmlChar*)tag);
	if(!nodeValue)
		return false;

	value = atoll(nodeValue);
	xmlFree(nodeValue);
	return true;
}

bool readXMLFloat(xmlNodePtr node, const char* tag, float& value)
{
	char* nodeValue = (char*)xmlGetProp(node, (xmlChar*)tag);
	if(!nodeValue)
		return false;

	value = atof(nodeValue);
	xmlFree(nodeValue);
	return true;
}

bool readXMLString(xmlNodePtr node, const char* tag, std::string& value)
{
	char* nodeValue = (char*)xmlGetProp(node, (xmlChar*)tag);
	if(!nodeValue)
		return false;

	if(!utf8ToLatin1(nodeValue, value))
		value = nodeValue;

	xmlFree(nodeValue);
	return true;
}

bool readXMLContentString(xmlNodePtr node, std::string& value)
{
	char* nodeValue = (char*)xmlNodeGetContent(node);
	if(!nodeValue)
		return false;

	if(!utf8ToLatin1(nodeValue, value))
		value = nodeValue;

	xmlFree(nodeValue);
	return true;
}

bool parseXMLContentString(xmlNodePtr node, std::string& value)
{
	bool result = false;
	std::string compareValue;
	while(node)
	{
		if(xmlStrcmp(node->name, (const xmlChar*)"text") && node->type != XML_CDATA_SECTION_NODE)
		{
			node = node->next;
			continue;
		}

		if(!readXMLContentString(node, compareValue))
		{
			node = node->next;
			continue;
		}

		trim_left(compareValue, "\r");
		trim_left(compareValue, "\n");
		trim_left(compareValue, " ");
		if(compareValue.length() > value.length())
		{
			value = compareValue;
			if(!result)
				result = true;
		}

		node = node->next;
	}

	return result;
}

std::string getLastXMLError()
{
	std::stringstream ss;
	xmlErrorPtr lastError = xmlGetLastError();
	if(lastError->line)
		ss << "Line: " << lastError->line << ", ";

	ss << "Info: " << lastError->message << std::endl;
	return ss.str();
}

bool utf8ToLatin1(char* intext, std::string& outtext)
{
	outtext = "";
	if(!intext)
		return false;

	int32_t inlen = strlen(intext);
	if(!inlen)
		return false;

	int32_t outlen = inlen * 2;
	uint8_t* outbuf = new uint8_t[outlen];

	int32_t res = UTF8Toisolat1(outbuf, &outlen, (uint8_t*)intext, &inlen);
	if(res < 0)
	{
		delete[] outbuf;
		return false;
	}

	outbuf[outlen] = '\0';
	outtext = (char*)outbuf;

	delete[] outbuf;
	return true;
}

StringVec explodeString(const std::string& string, const std::string& separator)
{
	StringVec returnVector;
	size_t start = 0, end = 0;
	while((end = string.find(separator, start)) != std::string::npos)
	{
		returnVector.push_back(string.substr(start, end - start));
		start = end + separator.size();
	}

	returnVector.push_back(string.substr(start));
	return returnVector;
}

IntegerVec vectorAtoi(StringVec stringVector)
{
	IntegerVec returnVector;
	for(StringVec::iterator it = stringVector.begin(); it != stringVector.end(); ++it)
		returnVector.push_back(atoi((*it).c_str()));

	return returnVector;
}

bool hasBitSet(uint32_t flag, uint32_t flags)
{
	return ((flags & flag) == flag);
}

int32_t round(float v)
{
	int32_t t = (int32_t)std::floor(v);
	if((v - t) > 0.5)
		return t + 1;

	return t;
}

uint32_t rand24b()
{
	return ((rand() << 12) ^ (rand())) & (0xFFFFFF);
}

float box_muller(float m, float s)
{
	// normal random variate generator
	// mean m, standard deviation s
	float x1, x2, w, y1;
	static float y2;

	static bool useLast = false;
	if(useLast) // use value from previous call
	{
		y1 = y2;
		useLast = false;
		return (m + y1 * s);
	}

	do
	{
		double r1 = (((float)(rand()) / RAND_MAX));
		double r2 = (((float)(rand()) / RAND_MAX));

		x1 = 2.0 * r1 - 1.0;
		x2 = 2.0 * r2 - 1.0;
		w = x1 * x1 + x2 * x2;
	}
	while(w >= 1.0);
	w = sqrt((-2.0 * log(w)) / w);

	y1 = x1 * w;
	y2 = x2 * w;

	useLast = true;
	return (m + y1 * s);
}

int32_t random_range(int32_t lowestNumber, int32_t highestNumber, DistributionType_t type /*= DISTRO_UNIFORM*/)
{
	if(highestNumber == lowestNumber)
		return lowestNumber;

	if(lowestNumber > highestNumber)
		std::swap(lowestNumber, highestNumber);

	switch(type)
	{
		case DISTRO_UNIFORM:
			return (lowestNumber + ((int32_t)rand24b() % (highestNumber - lowestNumber + 1)));
		case DISTRO_NORMAL:
			return (lowestNumber + int32_t(float(highestNumber - lowestNumber) * (float)std::min((float)1, std::max((float)0, box_muller(0.5, 0.25)))));
		default:
			break;
	}

	const float randMax = 16777216;
	return (lowestNumber + int32_t(float(highestNumber - lowestNumber) * float(1.f - sqrt((1.f * rand24b()) / randMax))));
}

char upchar(char character)
{
	if((character >= 97 && character <= 122) || (character <= -1 && character >= -32))
		character -= 32;

	return character;
}

bool isNumber(char character)
{
	return (character >= 48 && character <= 57);
}

bool isLowercaseLetter(char character)
{
	return (character >= 97 && character <= 122);
}

bool isUppercaseLetter(char character)
{
	return (character >= 65 && character <= 90);
}

bool isPasswordCharacter(char character)
{
	return ((character >= 33 && character <= 47) || (character >= 58 && character <= 64) || (character >= 91 && character <= 96) || (character >= 123 && character <= 126));
}

bool isValidAccountName(std::string text)
{
	toLowerCaseString(text);

	uint32_t textLength = text.length();
	for(uint32_t size = 0; size < textLength; size++)
	{
		if(!isLowercaseLetter(text[size]) && !isNumber(text[size]))
			return false;
	}

	return true;
}

bool isValidPassword(std::string text)
{
	toLowerCaseString(text);

	uint32_t textLength = text.length();
	for(uint32_t size = 0; size < textLength; size++)
	{
		if(!isLowercaseLetter(text[size]) && !isNumber(text[size]) && !isPasswordCharacter(text[size]))
			return false;
	}

	return true;
}

bool isValidName(std::string text, bool forceUppercaseOnFirstLetter/* = true*/)
{
	uint32_t textLength = text.length(), lenBeforeSpace = 1, lenBeforeQuote = 1, lenBeforeDash = 1, repeatedCharacter = 0;
	char lastChar = 32;
	if(forceUppercaseOnFirstLetter)
	{
		if(!isUppercaseLetter(text[0]))
			return false;
	}
	else if(!isLowercaseLetter(text[0]) && !isUppercaseLetter(text[0]))
		return false;

	for(uint32_t size = 1; size < textLength; size++)
	{
		if(text[size] != 32)
		{
			lenBeforeSpace++;

			if(text[size] != 39)
				lenBeforeQuote++;
			else
			{
				if(lenBeforeQuote <= 1 || size == textLength - 1 || text[size + 1] == 32)
					return false;

				lenBeforeQuote = 0;
			}

			if(text[size] != 45)
				lenBeforeDash++;
			else
			{
				if(lenBeforeDash <= 1 || size == textLength - 1 || text[size + 1] == 32)
					return false;

				lenBeforeDash = 0;
			}

			if(text[size] == lastChar)
			{
				repeatedCharacter++;
				if(repeatedCharacter > 2)
					return false;
			}
			else
				repeatedCharacter = 0;

			lastChar = text[size];
		}
		else
		{
			if(lenBeforeSpace <= 1 || size == textLength - 1 || text[size + 1] == 32)
				return false;

			lenBeforeSpace = lenBeforeQuote = lenBeforeDash = 0;
		}

		if(!(isLowercaseLetter(text[size]) || text[size] == 32 || text[size] == 39 || text[size] == 45
			|| (isUppercaseLetter(text[size]) && text[size - 1] == 32)))
			return false;
	}

	return true;
}

bool isNumbers(std::string text)
{
	uint32_t textLength = text.length();
	for(uint32_t size = 0; size < textLength; size++)
	{
		if(!isNumber(text[size]))
			return false;
	}

	return true;
}

bool checkText(std::string text, std::string str)
{
	trimString(text);
	return asLowerCaseString(text) == str;
}

std::string generateRecoveryKey(int32_t fieldCount, int32_t fieldLenght)
{
	std::stringstream key;
	int32_t i = 0, j = 0, lastNumber = 99, number = 0;

	char character = 0, lastCharacter = 0;
	bool madeNumber = false, madeCharacter = false;
	do
	{
		do
		{
			madeNumber = madeCharacter = false;
			if((bool)random_range(0, 1))
			{
				number = random_range(2, 9);
				if(number != lastNumber)
				{
					key << number;
					lastNumber = number;
					madeNumber = true;
				}
			}
			else
			{
				character = (char)random_range(65, 90);
				if(character != lastCharacter)
				{
					key << character;
					lastCharacter = character;
					madeCharacter = true;
				}
			}
		}
		while((!madeCharacter && !madeNumber) ? true : (++j && j < fieldLenght));
		lastCharacter = character = number = j = 0;

		lastNumber = 99;
		if(i < fieldCount - 1)
			key << "-";
	}
	while(++i && i < fieldCount);
	return key.str();
}

std::string trimString(std::string& str)
{
	str.erase(str.find_last_not_of(" ") + 1);
	return str.erase(0, str.find_first_not_of(" "));
}

std::string parseParams(tokenizer::iterator &it, tokenizer::iterator end)
{
	if(it == end)
		return "";

	std::string tmp = (*it);
	++it;
	if(tmp[0] == '"')
	{
		tmp.erase(0, 1);
		while(it != end && tmp[tmp.length() - 1] != '"')
		{
			tmp += " " + (*it);
			++it;
		}

		if(tmp.length() > 0 && tmp[tmp.length() - 1] == '"')
			tmp.erase(tmp.length() - 1);
	}

	return tmp;
}

std::string formatDate(time_t _time/* = 0*/)
{
	if(!_time)
		_time = time(NULL);

	const tm* tms = localtime(&_time);
	std::stringstream s;
	if(tms)
		s << tms->tm_mday << "/" << (tms->tm_mon + 1) << "/" << (tms->tm_year + 1900) << " " << tms->tm_hour << ":" << tms->tm_min << ":" << tms->tm_sec;
	else
		s << "UNIX Time: " << (int32_t)_time;

	return s.str();
}

std::string formatDateEx(time_t _time/* = 0*/, std::string format/* = "%d %b %Y, %H:%M:%S"*/)
{
	if(!_time)
		_time = time(NULL);

	const tm* tms = localtime(&_time);
	char buffer[100];
	if(tms)
		strftime(buffer, 25, format.c_str(), tms);
	else
		sprintf(buffer, "UNIX Time: %d", (int32_t)_time);

	return buffer;
}

std::string formatTime(time_t _time/* = 0*/, bool ms/* = false*/)
{
	if(!_time)
		_time = time(NULL);
	else if(ms)
		ms = false;

	const tm* tms = localtime(&_time);
	std::stringstream s;
	if(tms)
	{
		s << tms->tm_hour << ":" << tms->tm_min << ":";
		if(tms->tm_sec < 10)
			s << "0";

		s << tms->tm_sec;
		if(ms)
		{
			timeb t;
			ftime(&t);

			s << "."; // make it format zzz
			if(t.millitm < 10)
				s << "0";

			if(t.millitm < 100)
				s << "0";

			s << t.millitm;
		}
	}
	else
		s << "UNIX Time: " << (int32_t)_time;

	return s.str();
}

std::string convertIPAddress(uint32_t ip)
{
	char buffer[17];
	sprintf(buffer, "%d.%d.%d.%d", ip & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF, (ip >> 24));
	return buffer;
}

Skulls_t getSkull(std::string strValue)
{
	std::string tmpStrValue = asLowerCaseString(strValue);
	if(tmpStrValue == "red" || tmpStrValue == "4")
		return SKULL_RED;
	else if(tmpStrValue == "white" || tmpStrValue == "3")
		return SKULL_WHITE;
	else if(tmpStrValue == "green" || tmpStrValue == "2")
		return SKULL_GREEN;
	else if(tmpStrValue == "yellow" || tmpStrValue == "1")
		return SKULL_YELLOW;

	return SKULL_NONE;
}

PartyShields_t getPartyShield(std::string strValue)
{
	std::string tmpStrValue = asLowerCaseString(strValue);
	if(tmpStrValue == "yellow" || tmpStrValue == "4")
		return SHIELD_YELLOW;
	else if(tmpStrValue == "blue" || tmpStrValue == "3")
		return SHIELD_BLUE;
	else if(tmpStrValue == "whiteyellow" || tmpStrValue == "2")
		return SHIELD_WHITEYELLOW;
	else if(tmpStrValue == "whiteblue" || tmpStrValue == "1")
		return SHIELD_WHITEBLUE;

	return SHIELD_NONE;
}

Direction getDirection(std::string string)
{
	if(string == "north" || string == "n" || string == "0")
		return NORTH;
	else if(string == "east" || string == "e" || string == "1")
		return EAST;
	else if(string == "south" || string == "s" || string == "2")
		return SOUTH;
	else if(string == "west" || string == "w" || string == "3")
		return WEST;
	else if(string == "southwest" || string == "south west" || string == "south-west" || string == "sw" || string == "4")
		return SOUTHWEST;
	else if(string == "southeast" || string == "south east" || string == "south-east" || string == "se" || string == "5")
		return SOUTHEAST;
	else if(string == "northwest" || string == "north west" || string == "north-west" || string == "nw" || string == "6")
		return NORTHWEST;
	else if(string == "northeast" || string == "north east" || string == "north-east" || string == "ne" || string == "7")
		return NORTHEAST;

	return SOUTH;
}

Direction getDirectionTo(Position pos1, Position pos2, bool extended/* = true*/)
{
	Direction direction = NORTH;
	if(pos1.x > pos2.x)
	{
		direction = WEST;
		if(extended)
		{
			if(pos1.y > pos2.y)
				direction = NORTHWEST;
			else if(pos1.y < pos2.y)
				direction = SOUTHWEST;
		}
	}
	else if(pos1.x < pos2.x)
	{
		direction = EAST;
		if(extended)
		{
			if(pos1.y > pos2.y)
				direction = NORTHEAST;
			else if(pos1.y < pos2.y)
				direction = SOUTHEAST;
		}
	}
	else
	{
		if(pos1.y > pos2.y)
			direction = NORTH;
		else if(pos1.y < pos2.y)
			direction = SOUTH;
	}

	return direction;
}

Direction getReverseDirection(Direction dir)
{
	switch(dir)
	{
		case NORTH:
			return SOUTH;
		case SOUTH:
			return NORTH;
		case WEST:
			return EAST;
		case EAST:
			return WEST;
		case SOUTHWEST:
			return NORTHEAST;
		case NORTHWEST:
			return SOUTHEAST;
		case NORTHEAST:
			return SOUTHWEST;
		case SOUTHEAST:
			return NORTHWEST;
	}

	return SOUTH;
}

Position getNextPosition(Direction direction, Position pos)
{
	switch(direction)
	{
		case NORTH:
			pos.y--;
			break;
		case SOUTH:
			pos.y++;
			break;
		case WEST:
			pos.x--;
			break;
		case EAST:
			pos.x++;
			break;
		case SOUTHWEST:
			pos.x--;
			pos.y++;
			break;
		case NORTHWEST:
			pos.x--;
			pos.y--;
			break;
		case SOUTHEAST:
			pos.x++;
			pos.y++;
			break;
		case NORTHEAST:
			pos.x++;
			pos.y--;
			break;
	}

	return pos;
}

struct AmmoTypeNames
{
	const char* name;
	Ammo_t ammoType;
};

struct MagicEffectNames
{
	const char* name;
	MagicEffect_t magicEffect;
};

struct ShootTypeNames
{
	const char* name;
	ShootEffect_t shootType;
};

struct CombatTypeNames
{
	const char* name;
	CombatType_t combatType;
};

struct AmmoActionNames
{
	const char* name;
	AmmoAction_t ammoAction;
};

struct FluidTypeNames
{
	const char* name;
	FluidTypes_t fluidType;
};

struct SkillIdNames
{
	const char* name;
	skills_t skillId;
};

MagicEffectNames magicEffectNames[] =
{
	{"redspark",		MAGIC_EFFECT_DRAW_BLOOD},
	{"bluebubble",		MAGIC_EFFECT_LOSE_ENERGY},
	{"poff",		MAGIC_EFFECT_POFF},
	{"puff",		MAGIC_EFFECT_POFF},
	{"yellowspark",		MAGIC_EFFECT_BLOCKHIT},
	{"explosionarea",	MAGIC_EFFECT_EXPLOSION_AREA},
	{"explosion",		MAGIC_EFFECT_EXPLOSION_DAMAGE},
	{"firearea",		MAGIC_EFFECT_FIRE_AREA},
	{"yellowbubble",	MAGIC_EFFECT_YELLOW_RINGS},
	{"greenbubble",		MAGIC_EFFECT_POISON_RINGS},
	{"blackspark",		MAGIC_EFFECT_HIT_AREA},
	{"teleport",		MAGIC_EFFECT_TELEPORT},
	{"energyarea",		MAGIC_EFFECT_TELEPORT},
	{"energy",		MAGIC_EFFECT_ENERGY_DAMAGE},
	{"blueshimmer",		MAGIC_EFFECT_WRAPS_BLUE},
	{"redshimmer",		MAGIC_EFFECT_WRAPS_RED},
	{"greenshimmer",	MAGIC_EFFECT_WRAPS_GREEN},
	{"fire",		MAGIC_EFFECT_HITBY_FIRE},
	{"greenspark",		MAGIC_EFFECT_POISON},
	{"mortarea",		MAGIC_EFFECT_MORT_AREA},
	{"greennote",		MAGIC_EFFECT_SOUND_GREEN},
	{"rednote",		MAGIC_EFFECT_SOUND_RED},
	{"poison",		MAGIC_EFFECT_POISON_AREA},
	{"earth",		MAGIC_EFFECT_POISON_AREA},
	{"yellownote",		MAGIC_EFFECT_SOUND_YELLOW},
	{"purplenote",		MAGIC_EFFECT_SOUND_PURPLE},
	{"bluenote",		MAGIC_EFFECT_SOUND_BLUE},
	{"whitenote",		MAGIC_EFFECT_SOUND_WHITE},
	{"bubbles",		MAGIC_EFFECT_BUBBLES},
	{"dice",		MAGIC_EFFECT_CRAPS},
	{"giftwraps",		MAGIC_EFFECT_GIFT_WRAPS},
	{"yellowfirework",	MAGIC_EFFECT_FIREWORK_YELLOW},
	{"redfirework",		MAGIC_EFFECT_FIREWORK_RED},
	{"bluefirework",	MAGIC_EFFECT_FIREWORK_BLUE},
	{"stun",		MAGIC_EFFECT_STUN},
	{"sleep",		MAGIC_EFFECT_SLEEP},
	{"watercreature",	MAGIC_EFFECT_WATERCREATURE},
	{"groundshaker",	MAGIC_EFFECT_GROUNDSHAKER},
};

ShootTypeNames shootTypeNames[] =
{
	{"spear",		SHOOT_EFFECT_SPEAR},
	{"bolt",		SHOOT_EFFECT_BOLT},
	{"arrow",		SHOOT_EFFECT_ARROW},
	{"fire",		SHOOT_EFFECT_FIRE},
	{"energy",		SHOOT_EFFECT_ENERGY},
	{"poisonarrow",		SHOOT_EFFECT_POISONARROW},
	{"burstarrow",		SHOOT_EFFECT_BURSTARROW},
	{"throwingstar",	SHOOT_EFFECT_THROWINGSTAR},
	{"throwingknife",	SHOOT_EFFECT_THROWINGKNIFE},
	{"smallstone",		SHOOT_EFFECT_SMALLSTONE},
	{"death",		SHOOT_EFFECT_DEATH},
	{"suddendeath",		SHOOT_EFFECT_DEATH},
	{"largerock",		SHOOT_EFFECT_LARGEROCK},
	{"snowball",		SHOOT_EFFECT_SNOWBALL},
	{"powerbolt",		SHOOT_EFFECT_POWERBOLT},
	{"poison",		SHOOT_EFFECT_POISONFIELD},
	{"earth",		SHOOT_EFFECT_POISONFIELD},
	{"infernalbolt",	SHOOT_EFFECT_INFERNALBOLT},
	{"huntingspear",	SHOOT_EFFECT_HUNTINGSPEAR},
	{"enchantedspear",	SHOOT_EFFECT_ENCHANTEDSPEAR},
	{"redstar",		SHOOT_EFFECT_REDSTAR},
	{"greenstar",		SHOOT_EFFECT_GREENSTAR},
	{"royalspear",		SHOOT_EFFECT_ROYALSPEAR},
	{"sniperarrow",		SHOOT_EFFECT_SNIPERARROW},
	{"onyxarrow",		SHOOT_EFFECT_ONYXARROW},
	{"piercingbolt",	SHOOT_EFFECT_PIERCINGBOLT},
	{"whirlwindsword",	SHOOT_EFFECT_WHIRLWINDSWORD},
	{"whirlwindaxe",	SHOOT_EFFECT_WHIRLWINDAXE},
	{"whirlwindclub",	SHOOT_EFFECT_WHIRLWINDCLUB},
	{"etherealspear",	SHOOT_EFFECT_ETHEREALSPEAR}
};

CombatTypeNames combatTypeNames[] =
{
	{"physical",		COMBAT_PHYSICALDAMAGE},
	{"energy",		COMBAT_ENERGYDAMAGE},
	{"earth",		COMBAT_EARTHDAMAGE},
	{"poison",		COMBAT_EARTHDAMAGE},
	{"fire",		COMBAT_FIREDAMAGE},
	{"undefined",		COMBAT_UNDEFINEDDAMAGE},
	{"lifedrain",		COMBAT_LIFEDRAIN},
	{"life drain",		COMBAT_LIFEDRAIN},
	{"manadrain",		COMBAT_MANADRAIN},
	{"mana drain",		COMBAT_MANADRAIN},
	{"healing",		COMBAT_HEALING},
	{"drown",		COMBAT_DROWNDAMAGE}
};

AmmoTypeNames ammoTypeNames[] =
{
	{"spear",		AMMO_SPEAR},
	{"arrow",		AMMO_ARROW},
	{"poisonarrow",		AMMO_ARROW},
	{"burstarrow",		AMMO_ARROW},
	{"bolt",		AMMO_BOLT},
	{"powerbolt",		AMMO_BOLT},
	{"smallstone",		AMMO_STONE},
	{"largerock",		AMMO_STONE},
	{"throwingstar",	AMMO_THROWINGSTAR},
	{"throwingknife",	AMMO_THROWINGKNIFE},
	{"snowball",		AMMO_SNOWBALL},
	{"huntingspear",	AMMO_SPEAR},
	{"royalspear",		AMMO_SPEAR},
	{"enchantedspear",	AMMO_SPEAR},
	{"sniperarrow",		AMMO_ARROW},
	{"onyxarrow",		AMMO_ARROW},
	{"piercingbolt",	AMMO_BOLT},
	{"infernalbolt",	AMMO_BOLT},
	{"flasharrow",		AMMO_ARROW},
	{"flammingarrow",	AMMO_ARROW},
	{"flamingarrow",	AMMO_ARROW},
	{"shiverarrow",		AMMO_ARROW},
	{"eartharrow",		AMMO_ARROW},
	{"etherealspear",	AMMO_SPEAR}
};

AmmoActionNames ammoActionNames[] =
{
	{"move",		AMMOACTION_MOVE},
	{"moveback",		AMMOACTION_MOVEBACK},
	{"move back",		AMMOACTION_MOVEBACK},
	{"removecharge",	AMMOACTION_REMOVECHARGE},
	{"remove charge",	AMMOACTION_REMOVECHARGE},
	{"removecount",		AMMOACTION_REMOVECOUNT},
	{"remove count",	AMMOACTION_REMOVECOUNT}
};

FluidTypeNames fluidTypeNames[] =
{
	{"none",		FLUID_NONE},
	{"water",		FLUID_WATER},
	{"blood",		FLUID_BLOOD},
	{"beer",		FLUID_BEER},
	{"slime",		FLUID_SLIME},
	{"lemonade",		FLUID_LEMONADE},
	{"milk",		FLUID_MILK},
	{"mana",		FLUID_MANA},
	{"life",		FLUID_LIFE},
	{"oil",			FLUID_OIL},
	{"urine",		FLUID_URINE},
	{"coconutmilk",		FLUID_COCONUTMILK},
	{"coconut milk",	FLUID_COCONUTMILK},
	{"wine",		FLUID_WINE},
	{"mud",			FLUID_MUD},
	{"fruitjuice",		FLUID_FRUITJUICE},
	{"fruit juice",		FLUID_FRUITJUICE},
	{"lava",		FLUID_LAVA},
	{"rum",			FLUID_RUM},
	{"swamp",		FLUID_SWAMP}
};

SkillIdNames skillIdNames[] =
{
	{"fist",		SKILL_FIST},
	{"club",		SKILL_CLUB},
	{"sword",		SKILL_SWORD},
	{"axe",			SKILL_AXE},
	{"distance",		SKILL_DIST},
	{"dist",		SKILL_DIST},
	{"shielding",		SKILL_SHIELD},
	{"shield",		SKILL_SHIELD},
	{"fishing",		SKILL_FISH},
	{"fish",		SKILL_FISH},
	{"level",		SKILL__LEVEL},
	{"magiclevel",		SKILL__MAGLEVEL},
	{"magic level",		SKILL__MAGLEVEL}
};

MagicEffect_t getMagicEffect(const std::string& strValue)
{
	for(uint32_t i = 0; i < sizeof(magicEffectNames) / sizeof(MagicEffectNames); ++i)
	{
		if(!strcasecmp(strValue.c_str(), magicEffectNames[i].name))
			return magicEffectNames[i].magicEffect;
	}

	return MAGIC_EFFECT_UNKNOWN;
}

ShootEffect_t getShootType(const std::string& strValue)
{
	for(uint32_t i = 0; i < sizeof(shootTypeNames) / sizeof(ShootTypeNames); ++i)
	{
		if(!strcasecmp(strValue.c_str(), shootTypeNames[i].name))
			return shootTypeNames[i].shootType;
	}

	return SHOOT_EFFECT_UNKNOWN;
}

CombatType_t getCombatType(const std::string& strValue)
{
	for(uint32_t i = 0; i < sizeof(combatTypeNames) / sizeof(CombatTypeNames); ++i)
	{
		if(!strcasecmp(strValue.c_str(), combatTypeNames[i].name))
			return combatTypeNames[i].combatType;
	}

	return COMBAT_NONE;
}

Ammo_t getAmmoType(const std::string& strValue)
{
	for(uint32_t i = 0; i < sizeof(ammoTypeNames) / sizeof(AmmoTypeNames); ++i)
	{
		if(!strcasecmp(strValue.c_str(), ammoTypeNames[i].name))
			return ammoTypeNames[i].ammoType;
	}

	return AMMO_NONE;
}

AmmoAction_t getAmmoAction(const std::string& strValue)
{
	for(uint32_t i = 0; i < sizeof(ammoActionNames) / sizeof(AmmoActionNames); ++i)
	{
		if(!strcasecmp(strValue.c_str(), ammoActionNames[i].name))
			return ammoActionNames[i].ammoAction;
	}

	return AMMOACTION_NONE;
}

FluidTypes_t getFluidType(const std::string& strValue)
{
	for(uint32_t i = 0; i < sizeof(fluidTypeNames) / sizeof(FluidTypeNames); ++i)
	{
		if(!strcasecmp(strValue.c_str(), fluidTypeNames[i].name))
			return fluidTypeNames[i].fluidType;
	}

	return FLUID_NONE;
}

skills_t getSkillId(const std::string& strValue)
{
	for(uint32_t i = 0; i < sizeof(skillIdNames) / sizeof(SkillIdNames); ++i)
	{
		if(!strcasecmp(strValue.c_str(), skillIdNames[i].name))
			return skillIdNames[i].skillId;
	}

	return SKILL_FIST;
}

std::string getCombatName(CombatType_t combatType)
{
	switch(combatType)
	{
		case COMBAT_PHYSICALDAMAGE:
			return "physical";
		case COMBAT_ENERGYDAMAGE:
			return "energy";
		case COMBAT_EARTHDAMAGE:
			return "earth";
		case COMBAT_FIREDAMAGE:
			return "fire";
		case COMBAT_UNDEFINEDDAMAGE:
			return "undefined";
		case COMBAT_LIFEDRAIN:
			return "life drain";
		case COMBAT_MANADRAIN:
			return "mana drain";
		case COMBAT_HEALING:
			return "healing";
		case COMBAT_DROWNDAMAGE:
			return "drown";
		default:
			break;
	}

	return "unknown";
}

std::string getSkillName(uint16_t skillId, bool suffix/* = true*/)
{
	switch(skillId)
	{
		case SKILL_FIST:
		{
			std::string tmp = "fist";
			if(suffix)
				tmp += " fighting";

			return tmp;
		}
		case SKILL_CLUB:
		{
			std::string tmp = "club";
			if(suffix)
				tmp += " fighting";

			return tmp;
		}
		case SKILL_SWORD:
		{
			std::string tmp = "sword";
			if(suffix)
				tmp += " fighting";

			return tmp;
		}
		case SKILL_AXE:
		{
			std::string tmp = "axe";
			if(suffix)
				tmp += " fighting";

			return tmp;
		}
		case SKILL_DIST:
		{
			std::string tmp = "distance";
			if(suffix)
				tmp += " fighting";

			return tmp;
		}
		case SKILL_SHIELD:
			return "shielding";
		case SKILL_FISH:
			return "fishing";
		case SKILL__MAGLEVEL:
			return "magic level";
		case SKILL__LEVEL:
			return "level";
		default:
			break;
	}

	return "unknown";
}

std::string getReason(int32_t reasonId)
{
	switch(reasonId)
	{
		case 0: return "Offensive name"; break;
		case 1: return "Name containing part of sentece"; break;
		case 2: return "Name with nonsensical letter combo"; break;
		case 3: return "Invalid name format"; break;
		case 4: return "Name not describing person"; break;
		case 5: return "Name of celebrity"; break;
		case 6: return "Name referring to country"; break;
		case 7: return "Name to fake player identity"; break;
		case 8: return "Name to fake official position"; break;
		case 9: return "Offensive statement"; break;
		case 10: return "Spamming"; break;
		case 11: return "Advertisement not related to game"; break;
		case 12: return "Real money advertisement"; break;
		case 13: return "Non-English public statement"; break;
		case 14: return "Off-topic public statement"; break;
		case 15: return "Inciting rule violation"; break;
		case 16: return "Bug abuse"; break;
		case 17: return "Game weakness abuse"; break;
		case 18: return "Macro use"; break;
		case 19: return "Using unofficial software to play"; break;
		case 20: return "Hacking"; break;
		case 21: return "Multi-clienting"; break;
		case 22: return "Account trading"; break;
		case 23: return "Account sharing"; break;
		case 24: return "Threatening gamemaster"; break;
		case 25: return "Pretending to have official position"; break;
		case 26: return "Pretending to have influence on gamemaster"; break;
		case 27: return "False report to gamemaster"; break;
		case 28: return "Excessive unjustified player killing"; break;
		case 29: return "Destructive behaviour"; break;
		case 30: return "Spoiling auction"; break;
		case 31: return "Invalid payment"; break;
		default: return "Unknown reason"; break;
	}
}

std::string getAction(ViolationAction_t actionId, bool ipBanishment)
{
	std::string action = "Unknown";
	switch(actionId)
	{
		case ACTION_NOTATION:
			action = "Notation";
			break;
		case ACTION_NAMEREPORT:
			action = "Name Report";
			break;
		case ACTION_BANISHMENT:
			action = "Banishment";
			break;
		case ACTION_BANREPORT:
			action = "Name Report + Banishment";
			break;
		case ACTION_BANFINAL:
			action = "Banishment + Final Warning";
			break;
		case ACTION_BANREPORTFINAL:
			action = "Name Report + Banishment + Final Warning";
			break;
		case ACTION_STATEMENT:
			action = "Statement Report";
			break;
		//internal use
		case ACTION_DELETION:
			action = "Deletion";
			break;
		case ACTION_NAMELOCK:
			action = "Name Lock";
			break;
		case ACTION_BANLOCK:
			action = "Name Lock + Banishment";
			break;
		case ACTION_BANLOCKFINAL:
			action = "Name Lock + Banishment + Final Warning";
			break;
		default:
			break;
	}

	if(ipBanishment)
		action += " + IP Banishment";

	return action;
}

std::string parseVocationString(StringVec vocStringVec)
{
	std::string str = "";
	if(!vocStringVec.empty())
	{
		for(StringVec::iterator it = vocStringVec.begin(); it != vocStringVec.end(); ++it)
		{
			if((*it) != vocStringVec.front())
			{
				if((*it) != vocStringVec.back())
					str += ", ";
				else
					str += " and ";
			}

			str += (*it);
			str += "s";
		}
	}

	return str;
}

bool parseVocationNode(xmlNodePtr vocationNode, VocationMap& vocationMap, StringVec& vocStringVec, std::string& errorStr)
{
	if(xmlStrcmp(vocationNode->name,(const xmlChar*)"vocation"))
		return true;

	int32_t vocationId = -1;
	std::string strValue, tmpStrValue;
	if(readXMLString(vocationNode, "name", strValue))
	{
		vocationId = Vocations::getInstance()->getVocationId(strValue);
		if(vocationId != -1)
		{
			vocationMap[vocationId] = true;
			int32_t promotedVocation = Vocations::getInstance()->getPromotedVocation(vocationId);
			if(promotedVocation != -1)
				vocationMap[promotedVocation] = true;
		}
		else
		{
			errorStr = "Wrong vocation name: " + strValue;
			return false;
		}
	}
	else if(readXMLString(vocationNode, "id", strValue))
	{
		IntegerVec intVector;
		if(!parseIntegerVec(strValue, intVector))
		{
			errorStr = "Invalid vocation id - '" + strValue + "'";
			return false;
		}

		size_t size = intVector.size();
		for(size_t i = 0; i < size; ++i)
		{
			Vocation* vocation = Vocations::getInstance()->getVocation(intVector[i]);
			if(vocation && vocation->getName() != "")
			{
				vocationId = vocation->getId();
				strValue = vocation->getName();

				vocationMap[vocationId] = true;
				int32_t promotedVocation = Vocations::getInstance()->getPromotedVocation(vocationId);
				if(promotedVocation != -1)
					vocationMap[promotedVocation] = true;
			}
			else
			{
				std::stringstream ss;
				ss << "Wrong vocation id: " << intVector[i];

				errorStr = ss.str();
				return false;
			}
		}
	}

	if(vocationId != -1 && (!readXMLString(vocationNode, "showInDescription", tmpStrValue) || booleanString(tmpStrValue)))
		vocStringVec.push_back(asLowerCaseString(strValue));

	return true;
}

bool parseIntegerVec(std::string str, IntegerVec& intVector)
{
	StringVec strVector = explodeString(str, ";");
	IntegerVec tmpIntVector;
	for(StringVec::iterator it = strVector.begin(); it != strVector.end(); ++it)
	{
		tmpIntVector = vectorAtoi(explodeString((*it), "-"));
		if(!tmpIntVector[0] && it->substr(0, 1) != "0")
			continue;

		intVector.push_back(tmpIntVector[0]);
		if(tmpIntVector.size() > 1)
		{
			while(tmpIntVector[0] < tmpIntVector[1])
				intVector.push_back(++tmpIntVector[0]);
		}
	}

	return true;
}

bool fileExists(const char* filename)
{
	FILE* f = fopen(filename, "rb");
	if(!f)
		return false;

	fclose(f);
	return true;
}

uint32_t adlerChecksum(uint8_t *data, size_t length)
{
	if(length > NETWORKMESSAGE_MAXSIZE || length < 0)
		return 0;

	const uint16_t adler = 65521;
	uint32_t a = 1, b = 0;
	while(length > 0)
	{
		size_t tmp = length > 5552 ? 5552 : length;
		length -= tmp;
		do
		{
			a += *data++;
			b += a;
		}
		while(--tmp);

		a %= adler;
		b %= adler;
	}

	return (b << 16) | a;
}

std::string getFilePath(FileType_t type, std::string name/* = ""*/)
{
	#ifdef __FILESYSTEM_HIERARCHY_STANDARD__
	std::string path = "/usr/share/tfs/";
	#else
	std::string path = g_config.getString(ConfigManager::DATA_DIRECTORY);
	#endif
	switch(type)
	{
		case FILE_TYPE_OTHER:
			path += name;
			break;
		case FILE_TYPE_XML:
			path += "XML/" + name;
			break;
		case FILE_TYPE_LOG:
			#ifndef __FILESYSTEM_HIERARCHY_STANDARD__
			path = g_config.getString(ConfigManager::LOGS_DIRECTORY) + name;
			#else
			path = "/var/log/tfs/" + name;
			#endif
			break;
		case FILE_TYPE_MOD:
		{
			#ifndef __FILESYSTEM_HIERARCHY_STANDARD__
			path = "mods/" + name;
			#else
			path = "/etc/tfs/mods/" + name;
			#endif
			break;
		}
		case FILE_TYPE_CONFIG:
		{
			#if defined(__HOMEDIR_CONF__)
			if(fileExists("~/.tfs/" + name))
				path = "~/.tfs/" + name;
			else
			#endif
			#if defined(__FILESYSTEM_HIERARCHY_STANDARD__)
				path = "/etc/tcs/" + name;
			#else
				path = name;
			#endif
			break;
		}
		default:
			std::clog << ">> ERROR: Wrong file type!" << std::endl;
			break;
	}
	return path;
}

Skulls_t getSkulls(std::string strValue)
{
	std::string tmpStrValue = asLowerCaseString(strValue);
	if(tmpStrValue == "red" || tmpStrValue == "4")
		return SKULL_RED;

	if(tmpStrValue == "white" || tmpStrValue == "3")
		return SKULL_WHITE;

	if(tmpStrValue == "green" || tmpStrValue == "2")
		return SKULL_GREEN;

	if(tmpStrValue == "yellow" || tmpStrValue == "1")
		return SKULL_YELLOW;

	return SKULL_NONE;
}

PartyShields_t getShields(std::string strValue)
{
	std::string tmpStrValue = asLowerCaseString(strValue);
	if(tmpStrValue == "yellow" || tmpStrValue == "4")
		return SHIELD_YELLOW;

	if(tmpStrValue == "blue" || tmpStrValue == "3")
		return SHIELD_BLUE;

	if(tmpStrValue == "whiteyellow" || tmpStrValue == "2")
		return SHIELD_WHITEYELLOW;

	if(tmpStrValue == "whiteblue" || tmpStrValue == "1")
		return SHIELD_WHITEBLUE;

	return SHIELD_NONE;
}

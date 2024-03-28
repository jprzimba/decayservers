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

#ifndef __CONST__
#define __CONST__
#include "definitions.h"

enum MagicEffect_t
{
	MAGIC_EFFECT_DRAW_BLOOD	= 0x00,
	MAGIC_EFFECT_LOSE_ENERGY	= 0x01,
	MAGIC_EFFECT_POFF		= 0x02,
	MAGIC_EFFECT_BLOCKHIT		= 0x03,
	MAGIC_EFFECT_EXPLOSION_AREA	= 0x04,
	MAGIC_EFFECT_EXPLOSION_DAMAGE	= 0x05,
	MAGIC_EFFECT_FIRE_AREA		= 0x06,
	MAGIC_EFFECT_YELLOW_RINGS	= 0x07,
	MAGIC_EFFECT_POISON_RINGS	= 0x08,
	MAGIC_EFFECT_HIT_AREA		= 0x09,
	MAGIC_EFFECT_TELEPORT		= 0x0A, //10
	MAGIC_EFFECT_ENERGY_DAMAGE	= 0x0B, //11
	MAGIC_EFFECT_WRAPS_BLUE	= 0x0C, //12
	MAGIC_EFFECT_WRAPS_RED	= 0x0D, //13
	MAGIC_EFFECT_WRAPS_GREEN	= 0x0E, //14
	MAGIC_EFFECT_HITBY_FIRE	= 0x0F, //15
	MAGIC_EFFECT_POISON		= 0x10, //16
	MAGIC_EFFECT_MORT_AREA		= 0x11, //17
	MAGIC_EFFECT_SOUND_GREEN	= 0x12, //18
	MAGIC_EFFECT_SOUND_RED		= 0x13, //19
	MAGIC_EFFECT_POISON_AREA	= 0x14, //20
	MAGIC_EFFECT_SOUND_YELLOW	= 0x15, //21
	MAGIC_EFFECT_SOUND_PURPLE	= 0x16, //22
	MAGIC_EFFECT_SOUND_BLUE	= 0x17, //23
	MAGIC_EFFECT_SOUND_WHITE	= 0x18, //24
	MAGIC_EFFECT_BUBBLES		= 0x19, //25
	MAGIC_EFFECT_CRAPS		= 0x1A, //26
	MAGIC_EFFECT_GIFT_WRAPS	= 0x1B, //27
	MAGIC_EFFECT_FIREWORK_YELLOW	= 0x1C, //28
	MAGIC_EFFECT_FIREWORK_RED	= 0x1D, //29
	MAGIC_EFFECT_FIREWORK_BLUE	= 0x1E, //30
	MAGIC_EFFECT_STUN		= 0x1F, //31
	MAGIC_EFFECT_SLEEP		= 0x20, //32
	MAGIC_EFFECT_WATERCREATURE	= 0x21, //33
	MAGIC_EFFECT_GROUNDSHAKER	= 0x22, //34
	MAGIC_EFFECT_LAST		= MAGIC_EFFECT_GROUNDSHAKER,

	//for internal use, dont send to client
	MAGIC_EFFECT_NONE		= 0xFF,
	MAGIC_EFFECT_UNKNOWN		= 0xFFFF
};

enum ShootEffect_t
{
	SHOOT_EFFECT_SPEAR		= 0x00,
	SHOOT_EFFECT_BOLT		= 0x01,
	SHOOT_EFFECT_ARROW		= 0x02,
	SHOOT_EFFECT_FIRE		= 0x03,
	SHOOT_EFFECT_ENERGY		= 0x04,
	SHOOT_EFFECT_POISONARROW	= 0x05,
	SHOOT_EFFECT_BURSTARROW	= 0x06,
	SHOOT_EFFECT_THROWINGSTAR	= 0x07,
	SHOOT_EFFECT_THROWINGKNIFE	= 0x08,
	SHOOT_EFFECT_SMALLSTONE	= 0x09,
	SHOOT_EFFECT_DEATH		= 0x0A, //10
	SHOOT_EFFECT_LARGEROCK	= 0x0B, //11
	SHOOT_EFFECT_SNOWBALL	= 0x0C, //12
	SHOOT_EFFECT_POWERBOLT	= 0x0D, //13
	SHOOT_EFFECT_POISONFIELD	= 0x0E, //14
	SHOOT_EFFECT_INFERNALBOLT	= 0x0F, //15
	SHOOT_EFFECT_HUNTINGSPEAR	= 0x10, //16
	SHOOT_EFFECT_ENCHANTEDSPEAR	= 0x11, //17
	SHOOT_EFFECT_REDSTAR	= 0x12, //18
	SHOOT_EFFECT_GREENSTAR	= 0x13, //19
	SHOOT_EFFECT_ROYALSPEAR	= 0x14, //20
	SHOOT_EFFECT_SNIPERARROW	= 0x15, //21
	SHOOT_EFFECT_ONYXARROW	= 0x16, //22
	SHOOT_EFFECT_PIERCINGBOLT	= 0x17, //23
	SHOOT_EFFECT_WHIRLWINDSWORD	= 0x18, //24
	SHOOT_EFFECT_WHIRLWINDAXE	= 0x19, //25
	SHOOT_EFFECT_WHIRLWINDCLUB	= 0x1A, //26
	SHOOT_EFFECT_ETHEREALSPEAR	= 0x1B, //27
	SHOOT_EFFECT_LAST		= SHOOT_EFFECT_ETHEREALSPEAR,

	//for internal use, dont send to client
	SHOOT_EFFECT_WEAPONTYPE	= 0xFE, //254
	SHOOT_EFFECT_NONE		= 0xFF,
	SHOOT_EFFECT_UNKNOWN	= 0xFFFF
};

enum SpeakClasses
{
	SPEAK_CLASS_NONE	= 0x00,
	SPEAK_CLASS_FIRST 	= 0x01,
	SPEAK_SAY		= SPEAK_CLASS_FIRST,
	SPEAK_WHISPER		= 0x02,
	SPEAK_YELL		= 0x03,
	SPEAK_PRIVATE		= 0x04,
	SPEAK_CHANNEL_Y		= 0x05, //yellow
	SPEAK_RVR_CHANNEL   = 0x06,
    SPEAK_RVR_ANSWER    = 0x07,
    SPEAK_RVR_CONTINUE  = 0x08,
	SPEAK_BROADCAST		= 0x09,
	SPEAK_CHANNEL_RN	= 0x0A, //red - #c text
	SPEAK_PRIVATE_RED	= 0x0B,	//@name@text
	SPEAK_CHANNEL_O		= 0x0C, //orange
    //SPEAK_            = 0x0D,
    SPEAK_CHANNEL_RA    = 0x0E,	//red anonymous - #d text
    //SPEAK_            = 0x0F,
	SPEAK_MONSTER_SAY	= 0x10,
	SPEAK_MONSTER_YELL	= 0x11,
	SPEAK_CLASS_LAST 	= SPEAK_MONSTER_YELL
};

enum MessageClasses
{
	MSG_CLASS_FIRST			= 0x01,
	MSG_STATUS_CONSOLE_YELLOW       = MSG_CLASS_FIRST, //Yellow message in the console
    MSG_STATUS_CONSOLE_TEAL        = 0x04, //Light blue message in the console
	MSG_STATUS_CONSOLE_ORANGE	= 0x11, /*Orange message in the console*/
	MSG_STATUS_WARNING		= 0x12, /*Red message in game window and in the console*/
	MSG_EVENT_ADVANCE		= 0x13, /*White message in game window and in the console*/
	MSG_EVENT_DEFAULT		= 0x14, /*White message at the bottom of the game window and in the console*/
	MSG_STATUS_DEFAULT		= 0x15, /*White message at the bottom of the game window and in the console*/
	MSG_INFO_DESCR			= 0x16, /*Green message in game window and in the console*/
	MSG_STATUS_SMALL		= 0x17, /*White message at the bottom of the game window"*/
	MSG_STATUS_CONSOLE_BLUE		= 0x18, /*Blue message in the console*/
	MSG_STATUS_CONSOLE_RED		= 0x19, /*Red message in the console*/
	MSG_CLASS_LAST			= MSG_STATUS_CONSOLE_RED
};

enum MapMarks_t
{
	MAPMARK_TICK		= 0x00,
	MAPMARK_QUESTION	= 0x01,
	MAPMARK_EXCLAMATION	= 0x02,
	MAPMARK_STAR		= 0x03,
	MAPMARK_CROSS		= 0x04,
	MAPMARK_TEMPLE		= 0x05,
	MAPMARK_KISS		= 0x06,
	MAPMARK_SHOVEL		= 0x07,
	MAPMARK_SWORD		= 0x08,
	MAPMARK_FLAG		= 0x09,
	MAPMARK_LOCK		= 0x0A,
	MAPMARK_BAG		= 0x0B,
	MAPMARK_SKULL		= 0x0C,
	MAPMARK_DOLLAR		= 0x0D,
	MAPMARK_REDNORTH	= 0x0E,
	MAPMARK_REDSOUTH	= 0x0F,
	MAPMARK_REDEAST		= 0x10,
	MAPMARK_REDWEST		= 0x11,
	MAPMARK_GREENNORTH	= 0x12,
	MAPMARK_GREENSOUTH	= 0x13
};

enum FluidColors_t
{
	FLUID_EMPTY	= 0x00,
	FLUID_BLUE	= 0x01,
	FLUID_RED	= 0x02,
	FLUID_BROWN	= 0x03,
	FLUID_GREEN	= 0x04,
	FLUID_YELLOW	= 0x05,
	FLUID_WHITE	= 0x06,
	FLUID_PURPLE	= 0x07
};

enum FluidTypes_t
{
	FLUID_NONE		= FLUID_EMPTY,
	FLUID_WATER		= FLUID_BLUE,
	FLUID_BLOOD		= FLUID_RED,
	FLUID_BEER		= FLUID_BROWN,
	FLUID_SLIME		= FLUID_GREEN,
	FLUID_LEMONADE		= FLUID_YELLOW,
	FLUID_MILK		= FLUID_WHITE,
	FLUID_MANA		= FLUID_PURPLE,

	FLUID_LIFE		= FLUID_RED + 8,
	FLUID_OIL		= FLUID_BROWN + 8,
	FLUID_URINE		= FLUID_YELLOW + 8,
	FLUID_COCONUTMILK	= FLUID_WHITE + 8,
	FLUID_WINE		= FLUID_PURPLE + 8,

	FLUID_MUD		= FLUID_BROWN + 16,
	FLUID_FRUITJUICE	= FLUID_YELLOW + 16,

	FLUID_LAVA		= FLUID_RED + 24,
	FLUID_RUM		= FLUID_BROWN + 24,
	FLUID_SWAMP		= FLUID_GREEN + 24,
};

const uint8_t reverseFluidMap[] =
{
	FLUID_EMPTY,
	FLUID_WATER,
	FLUID_MANA,
	FLUID_BEER,
	FLUID_EMPTY,
	FLUID_BLOOD,
	FLUID_SLIME,
	FLUID_EMPTY,
	FLUID_LEMONADE,
	FLUID_MILK
};

enum ClientFluidTypes_t
{
	CLIENTFLUID_EMPTY	= 0x00,
	CLIENTFLUID_BLUE	= 0x01,
	CLIENTFLUID_PURPLE	= 0x02,
	CLIENTFLUID_BROWN_1	= 0x03,
	CLIENTFLUID_BROWN_2	= 0x04,
	CLIENTFLUID_RED		= 0x05,
	CLIENTFLUID_GREEN	= 0x06,
	CLIENTFLUID_BROWN	= 0x07,
	CLIENTFLUID_YELLOW	= 0x08,
	CLIENTFLUID_WHITE	= 0x09
};

const uint8_t fluidMap[] =
{
	CLIENTFLUID_EMPTY,
	CLIENTFLUID_BLUE,
	CLIENTFLUID_RED,
	CLIENTFLUID_BROWN_1,
	CLIENTFLUID_GREEN,
	CLIENTFLUID_YELLOW,
	CLIENTFLUID_WHITE,
	CLIENTFLUID_PURPLE
};

enum SquareColor_t
{
	SQ_COLOR_NONE = 256,
	SQ_COLOR_BLACK = 0,
};

enum TextColor_t
{
	TEXTCOLOR_BLUE        = 5,
	TEXTCOLOR_LIGHTBLUE   = 35,
	TEXTCOLOR_GREEN  = 30,
	TEXTCOLOR_GREY   = 129,
	TEXTCOLOR_RED         = 180,
	TEXTCOLOR_ORANGE      = 198,
	TEXTCOLOR_WHITE   = 215,
	TEXTCOLOR_NONE        = 255,
	TEXTCOLOR_UNKNOWN	= 256
};

enum Icons_t
{
	ICON_NONE = 0,
	ICON_POISON = 1 << 0,
	ICON_BURN = 1 << 1,
	ICON_ENERGY = 1 << 2,
	ICON_DRUNK = 1 << 3,
	ICON_MANASHIELD = 1 << 4,
	ICON_PARALYZE = 1 << 5,
	ICON_HASTE = 1 << 6,
	ICON_SWORDS = 1 << 7,
	ICON_DROWNING = 1 << 8
};

enum WeaponType_t
{
	WEAPON_NONE = 0,
	WEAPON_SWORD = 1,
	WEAPON_CLUB = 2,
	WEAPON_AXE = 3,
	WEAPON_SHIELD = 4,
	WEAPON_DIST = 5,
	WEAPON_WAND = 6,
	WEAPON_AMMO = 7,
	WEAPON_FIST = 8
};

enum Ammo_t
{
	AMMO_NONE = 0,
	AMMO_BOLT = 1,
	AMMO_ARROW = 2,
	AMMO_SPEAR = 3,
	AMMO_THROWINGSTAR = 4,
	AMMO_THROWINGKNIFE = 5,
	AMMO_STONE = 6,
	AMMO_SNOWBALL = 7
};

enum AmmoAction_t
{
	AMMOACTION_NONE,
	AMMOACTION_REMOVECOUNT,
	AMMOACTION_REMOVECHARGE,
	AMMOACTION_MOVE,
	AMMOACTION_MOVEBACK
};

enum WieldInfo_t
{
	WIELDINFO_LEVEL = 1,
	WIELDINFO_MAGLV = 2,
	WIELDINFO_VOCREQ = 4,
	WIELDINFO_PREMIUM = 8
};

enum Skulls_t
{
	SKULL_NONE = 0,
	SKULL_YELLOW = 1,
	SKULL_GREEN = 2,
	SKULL_WHITE = 3,
	SKULL_RED = 4,
	SKULL_LAST = SKULL_RED
};

enum PartyShields_t
{
	SHIELD_NONE = 0,
	SHIELD_WHITEYELLOW = 1,
	SHIELD_WHITEBLUE = 2,
	SHIELD_BLUE = 3,
	SHIELD_YELLOW = 4
};

enum item_t
{
	ITEM_FIREFIELD		= 1492,
	ITEM_FIREFIELD_SAFE	= 1500,

	ITEM_POISONFIELD	= 1496,
	ITEM_POISONFIELD_SAFE	= 1503,

	ITEM_ENERGYFIELD	= 1495,
	ITEM_ENERGYFIELD_SAFE	= 1504,

	ITEM_DEPOT		= 2594,
	ITEM_LOCKER		= 2589,

	ITEM_MALE_CORPSE	= 3058,
	ITEM_FEMALE_CORPSE	= 3065,

	ITEM_MEAT		= 2666,
	ITEM_HAM		= 2671,
	ITEM_GRAPE		= 2681,
	ITEM_APPLE		= 2674,
	ITEM_BREAD		= 2689,
	ITEM_ROLL		= 2690,
	ITEM_CHEESE		= 2696,

	ITEM_FULLSPLASH		= 2016,
	ITEM_SMALLSPLASH	= 2019,

	ITEM_PARCEL		= 2595,
	ITEM_PARCEL_STAMPED	= 2596,
	ITEM_LETTER		= 2597,
	ITEM_LETTER_STAMPED	= 2598,
	ITEM_LABEL		= 2599,

	ITEM_HOUSE_TRANSFER	= 1968 //read-only
};

enum PlayerFlags
{
	PlayerFlag_CannotUseCombat = 0,			//2^0 = 1
	PlayerFlag_CannotAttackPlayer,			//2^1 = 2
	PlayerFlag_CannotAttackMonster,			//2^2 = 4
	PlayerFlag_CannotBeAttacked,			//2^3 = 8
	PlayerFlag_CanConvinceAll,			//2^4 = 16
	PlayerFlag_CanSummonAll,			//2^5 = 32
	PlayerFlag_CanIllusionAll,			//2^6 = 64
	PlayerFlag_CanSenseInvisibility,		//2^7 = 128
	PlayerFlag_IgnoredByMonsters,			//2^8 = 256
	PlayerFlag_NotGainInFight,			//2^9 = 512
	PlayerFlag_HasInfiniteMana,			//2^10 = 1024
	PlayerFlag_HasInfiniteSoul,			//2^11 = 2048
	PlayerFlag_HasNoExhaustion,			//2^12 = 4096
	PlayerFlag_CannotUseSpells,			//2^13 = 8192
	PlayerFlag_CannotPickupItem,			//2^14 = 16384
	PlayerFlag_CanAlwaysLogin,			//2^15 = 32768
	PlayerFlag_CanBroadcast,			//2^16 = 65536
	PlayerFlag_CanEditHouses,			//2^17 = 131072
	PlayerFlag_CannotBeBanned,			//2^18 = 262144
	PlayerFlag_CannotBePushed,			//2^19 = 524288
	PlayerFlag_HasInfiniteCapacity,			//2^20 = 1048576
	PlayerFlag_CanPushAllCreatures,			//2^21 = 2097152
	PlayerFlag_CanTalkRedPrivate,			//2^22 = 4194304
	PlayerFlag_CanTalkRedChannel,			//2^23 = 8388608
	PlayerFlag_TalkOrangeHelpChannel,		//2^24 = 16777216
	PlayerFlag_NotGainExperience,			//2^25 = 33554432
	PlayerFlag_NotGainMana,				//2^26 = 67108864
	PlayerFlag_NotGainHealth,			//2^27 = 134217728
	PlayerFlag_NotGainSkill,			//2^28 = 268435456
	PlayerFlag_SetMaxSpeed,				//2^29 = 536870912
	PlayerFlag_SpecialVIP,				//2^30 = 1073741824
	PlayerFlag_NotGenerateLoot,			//2^31 = 2147483648
	PlayerFlag_CanTalkRedChannelAnonymous,		//2^32 = 4294967296
	PlayerFlag_IgnoreProtectionZone,		//2^33 = 8589934592
	PlayerFlag_IgnoreSpellCheck,			//2^34 = 17179869184
	PlayerFlag_IgnoreEquipCheck,			//2^35 = 34359738368
	PlayerFlag_CannotBeMuted,			//2^36 = 68719476736
	PlayerFlag_IsAlwaysPremium,			//2^37 = 137438953472
	PlayerFlag_CanAnswerRuleViolations,		//2^38 = 274877906944
	PlayerFlag_39,	//ignore			//2^39 = 549755813888	//not used by us
	PlayerFlag_ShowGroupNameInsteadOfVocation,	//2^40 = 1099511627776
	PlayerFlag_HasInfiniteStamina,			//2^41 = 2199023255552
	PlayerFlag_CannotMoveItems,			//2^42 = 4398046511104
	PlayerFlag_CannotMoveCreatures,			//2^43 = 8796093022208
	PlayerFlag_CanReportBugs,			//2^44 = 17592186044416
	PlayerFlag_45,	//ignore			//2^45 = 35184372088832	//not used by us
	PlayerFlag_CannotBeSeen,			//2^46 = 70368744177664

	PlayerFlag_LastFlag
};

enum PlayerCustomFlags
{
	PlayerCustomFlag_AllowIdle = 0,				//2^0 = 1
	PlayerCustomFlag_CanSeePosition,			//2^1 = 2
	PlayerCustomFlag_CanSeeItemDetails,			//2^2 = 4
	PlayerCustomFlag_CanSeeCreatureDetails,			//2^3 = 8
	PlayerCustomFlag_NotSearchable,				//2^4 = 16
	PlayerCustomFlag_GamemasterPrivileges,			//2^5 = 32
	PlayerCustomFlag_CanThrowAnywhere,			//2^6 = 64
	PlayerCustomFlag_CanPushAllItems,			//2^7 = 128
	PlayerCustomFlag_CanMoveAnywhere,			//2^8 = 256
	PlayerCustomFlag_CanMoveFromFar,			//2^9 = 512
	PlayerCustomFlag_CanLoginMultipleCharacters,		//2^10 = 1024 (account flag)
	PlayerCustomFlag_HasFullLight,				//2^11 = 2048
	PlayerCustomFlag_CanLogoutAnytime,			//2^12 = 4096 (account flag)
	PlayerCustomFlag_HideLevel,				//2^13 = 8192
	PlayerCustomFlag_IsProtected,				//2^14 = 16384
	PlayerCustomFlag_IsImmune,				//2^15 = 32768
	PlayerCustomFlag_NotGainSkull,				//2^16 = 65536
	PlayerCustomFlag_NotGainUnjustified,			//2^17 = 131072
	PlayerCustomFlag_IgnorePacification,			//2^18 = 262144
	PlayerCustomFlag_IgnoreLoginDelay,			//2^19 = 524288
	PlayerCustomFlag_CanStairhop,				//2^20 = 1048576
	PlayerCustomFlag_CanTurnhop,				//2^21 = 2097152
	PlayerCustomFlag_IgnoreHouseRent,			//2^22 = 4194304
	PlayerCustomFlag_CanWearAllAddons,			//2^23 = 8388608

	PlayerCustomFlag_LastFlag
};

//Reserved player storage key ranges
//[10000000 - 20000000]
#define PSTRG_RESERVED_RANGE_START	10000000
#define PSTRG_RESERVED_RANGE_SIZE	10000000

//[1000 - 1500]
#define PSTRG_OUTFITS_RANGE_START	(PSTRG_RESERVED_RANGE_START + 1000)
#define PSTRG_OUTFITS_RANGE_SIZE	500

//[1500 - 2000]
#define PSTRG_OUTFITSID_RANGE_START	(PSTRG_RESERVED_RANGE_START + 1500)
#define PSTRG_OUTFITSID_RANGE_SIZE	500

#define NETWORKMESSAGE_MAXSIZE 15360
#define IPBAN_FLAG 128
#define LOCALHOST 2130706433

#define GRATIS_PREMIUM 65535

#define IS_IN_KEYRANGE(key, range) (key >= PSTRG_##range##_START && ((key - PSTRG_##range##_START) < PSTRG_##range##_SIZE))
#endif

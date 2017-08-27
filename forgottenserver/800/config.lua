-- The Forgotten Server Config

	-- Account manager
	accountManager = true
	namelockManager = true
	newPlayerChooseVoc = false
	newPlayerSpawnPosX = 95
	newPlayerSpawnPosY = 117
	newPlayerSpawnPosZ = 7
	newPlayerTownId = 1
	newPlayerLevel = 1
	newPlayerMagicLevel = 0
	generateAccountNumber = false

	-- Unjustified kills
	-- Auto banishing works.
	-- advancedFragList is not advised if you use huge frags
	-- requirements.
	redSkullLength = 30 * 24 * 60 * 60
	dailyFragsToRedSkull = 3
	weeklyFragsToRedSkull = 5
	monthlyFragsToRedSkull = 10
	dailyFragsToBanishment = dailyFragsToRedSkull
	weeklyFragsToBanishment = weeklyFragsToRedSkull
	monthlyFragsToBanishment = monthlyFragsToRedSkull
	useFragHandler = true
	advancedFragList = false

	-- Banishments
	-- violationNameReportActionType 1 = just a report, 2 = name lock, 3 = player banishment
	notationsToBan = 3
	warningsToFinalBan = 4
	warningsToDeletion = 5
	banLength = 7 * 24 * 60 * 60
	killsBanLength = 7 * 24 * 60 * 60
	finalBanLength = 30 * 24 * 60 * 60
	ipBanishmentLength = 1 * 24 * 60 * 60
	broadcastBanishments = true
	maxViolationCommentSize = 200
	violationNameReportActionType = 2
	autoBanishUnknownBytes = false

	-- Battle
	-- NOTE: showHealingDamageForMonsters inheritates from showHealingDamage.
	-- loginProtectionPeriod is the famous Tibia anti-magebomb system.
	-- deathLostPercent set to nil enables manual mode.
	worldType = "pvp"
	protectionLevel = 1
	pvpTileIgnoreLevelAndVocationProtection = true
	pzLocked = 60 * 1000
	huntingDuration = 60 * 1000
	criticalHitChance = 7
	criticalHitMultiplier = 1
	displayCriticalHitNotify = false
	removeWeaponAmmunition = true
	removeWeaponCharges = true
	removeRuneCharges = true
	whiteSkullTime = 15 * 60 * 1000
	noDamageToSameLookfeet = false
	showHealingDamage = false
	showHealingDamageForMonsters = false
	fieldOwnershipDuration = 5 * 1000
	stopAttackingAtExit = false
	oldConditionAccuracy = false
	loginProtectionPeriod = 10 * 1000
	deathLostPercent = 10
	stairhopDelay = 2 * 1000
	pushCreatureDelay = 2 * 1000
	deathContainerId = 1987
	gainExperienceColor = 215
	addManaSpentInPvPZone = true
	squareColor = 0
	allowFightback = true
	fistBaseAttack = 7

	-- Connection config
	worldId = 0
	ip = "127.0.0.1"
	loginPort = 7171
	loginTries = 10
	retryTimeout = 5 * 1000
	loginTimeout = 60 * 1000
	maxPlayers = 1000
	motd = "Welcome to the Forgotten Server!"
	displayOnOrOffAtCharlist = false
	onePlayerOnlinePerAccount = true
	allowClones = false
	serverName = "Forgotten"
	loginMessage = "Welcome to the Forgotten Server!"
	statusTimeout = 5 * 60 * 1000
	replaceKickOnLogin = true
	premiumPlayerSkipWaitList = false

	-- Database
	-- NOTE: sqlFile is used only by sqlite database, and sqlKeepAlive by mysql database.
	-- To disable sqlKeepAlive such as mysqlReadTimeout use 0 value.
	sqlType = "mysql"
	sqlHost = "127.0.0.1"
	sqlPort = 3306
	sqlUser = "otserv"
	sqlPass = ""
	sqlDatabase = "tfs"
	sqlFile = "forgottenserver.s3db"
	sqlKeepAlive = 0
	mysqlReadTimeout = 10
	mysqlWriteTimeout = 10
	encryptionType = "plain"

	-- Deathlist
	deathListEnabled = true
	deathListRequiredTime = 1 * 60 * 1000
	deathAssistCount = 19
	maxDeathRecords = 5
	multipleNames = false

	-- Guilds
	ingameGuildManagement = true
	levelToFormGuild = 8
	premiumDaysToFormGuild = 0
	guildNameMinLength = 4
	guildNameMaxLength = 20

	-- Highscores
	highscoreDisplayPlayers = 15
	updateHighscoresAfterMinutes = 60

	-- Houses
	buyableAndSellableHouses = true
	houseNeedPremium = true
	bedsRequirePremium = true
	levelToBuyHouse = 1
	housesPerAccount = 0
	houseRentAsPrice = false
	housePriceAsRent = false
	housePriceEachSquare = 1000
	houseSkipInitialRent = true
	houseRentPeriod = "never"
	houseCleanOld = 0
	guildHalls = false
	houseProtection = true

	-- Item usage
	timeBetweenActions = 200
	timeBetweenExActions = 1000
	hotkeyAimbotEnabled = true

	-- Map
	-- NOTE: storeTrash costs more memory, but will perform alot faster cleaning.
	mapName = "forgotten"
	mapAuthor = "Komic"
	randomizeTiles = true
	storeTrash = true
	cleanProtectedZones = true

	-- Mailbox
	mailboxDisabledTowns = ""
	mailMaxAttempts = 20
	mailBlockPeriod = 60 * 60 * 1000
	mailAttemptsFadeTime = 10 * 60 * 1000

	-- Process
	-- NOTE: daemonize works only on *nix, same as niceLevel, while
	-- defaultPriority works only on Windows.
	-- coresUsed are seperated by comma cores ids used by server process,
	-- default is -1, so it stays untouched (automaticaly assigned by OS).
	daemonize = false
	defaultPriority = "normal"
	niceLevel = 5
	coresUsed = "-1"

	-- Startup
	startupDatabaseOptimization = true
	updatePremiumStateAtStartup = true
	skipItemsVersionCheck = false

	-- Spells
	classicSpells = true
	useRunesRequirements = true
	formulaLevel = 5.0
	formulaMagic = 1.0
	bufferMutedOnSpellFailure = false
	spellNameInsteadOfWords = false
	emoteSpells = false
	unifiedSpells = true

	-- Outfits
	allowChangeOutfit = true
	allowChangeColors = true
	allowChangeAddons = true
	disableOutfitsForPrivilegedPlayers = false
	addonsOnlyPremium = true

	-- Miscellaneous
	-- NOTE: promptExceptionTracerErrorBox works only with precompiled support feature,
	-- called "exception tracer" (__EXCEPTION_TRACER__ flag).
	dataDirectory = "data/"
	logsDirectory = "data/logs/"
	bankSystem = true
	displaySkillLevelOnAdvance = false
	promptExceptionTracerErrorBox = true
	maximumDoorLevel = 500
	maxMessageBuffer = 4
	tradeLimit = 100
	useCapacity = true

	-- Depot
	defaultDepotSizePremium = 2000
	defaultDepotSize = 1000

	-- VIP list
	separateVipListPerCharacter = false
	vipListDefaultLimit = 20
	vipListDefaultPremiumLimit = 100

	-- Saving-related
	-- houseDataStorage, you can use binary and/or relational values.
	houseDataStorage = "binary"
	saveGlobalStorage = true
	storePlayerDirection = false
	savePlayerData = true

	-- Loot
	-- monsterLootMessage 0 to disable, 1 - only party, 2 - only player, 3 - party or player (like Tibia's)
	checkCorpseOwner = true
	monsterLootMessage = 3
	monsterLootMessageType = 22

	-- Ghost mode
	ghostModeInvisibleEffect = false
	ghostModeSpellEffects = true

	-- Limits
	idleWarningTime = 14 * 60 * 1000
	idleKickTime = 15 * 60 * 1000
	expireReportsAfterReads = 1
	playerQueryDeepness = -1
	tileLimit = 0
	protectionTileLimit = 0
	houseTileLimit = 0

	-- Premium-related
	freePremium = false
	premiumForPromotion = true

	-- Blessings
	-- NOTE: blessingReduction* regards items/containers loss.
	-- eachBlessReduction is how much each bless reduces the experience/magic/skills loss.
	blessings = true
	blessingOnlyPremium = true
	blessingReductionBase = 30
	blessingReductionDecreament = 5
	eachBlessReduction = 8

	-- Rates
	-- NOTE: experienceStages configuration is located in data/XML/stages.xml.
	-- rateExperienceFromPlayers 0 to disable.
	-- For constant spawn count increment, use exact values in rateSpawn* variables.
	experienceStages = false
	rateExperience = 5.0
	rateExperienceFromPlayers = 0
	rateSkill = 3.0
	rateMagic = 3.0
	rateLoot = 2.0
	rateSpawnMin = 1
	rateSpawnMax = 1

	-- Monster rates
	rateMonsterHealth = 1.0
	rateMonsterMana = 1.0
	rateMonsterAttack = 1.0
	rateMonsterDefense = 1.0

	-- Experience from players
	-- NOTE: min~Threshold* set to 0 will disable the minimum threshold:
	-- player will gain experience from every lower leveled player.
	-- max~Threshold* set to 0 will disable the maximum threshold:
	-- player will gain experience from every higher leveled player.
	minLevelThresholdForKilledPlayer = 0.9
	maxLevelThresholdForKilledPlayer = 1.1

	-- Stamina
	-- NOTE: Stamina is stored in miliseconds, so seconds are multiplied by 1000.
	-- rateStaminaHits multiplies every hit done a creature, which are later
	-- multiplied by player attack speed.
	-- rateStaminaGain is divider of every logged out second, eg:
	-- 60000 / 3 = 20000 milliseconds, what gives 20 stamina seconds for 1 minute being logged off.
	-- rateStaminaThresholdGain is dividing in case the normal gain (that is
	-- multiplied by rateStaminaGain, btw.) passed above threshold, eg:
	-- 60 * 1000 / 3 = 20 / 4 = 5 seconds (3 * 4 = 12 minutes for 1 stamina minute).
	-- staminaRatingLimit* is in minutes.
	-- don't change notting here if you don't know
	rateStaminaLoss = 1
	rateStaminaGain = 3
	rateStaminaThresholdGain = 12
	staminaRatingLimitTop = 55 * 60
	staminaRatingLimitBottom = 14 * 60
	staminaLootLimit = 14 * 60
	rateStaminaAboveNormal = 1.5
	rateStaminaUnderNormal = 0.5
	staminaThresholdOnlyPremium = true

	-- Global save
	-- NOTE: globalSaveHour means like 03:00, not that it will save every 3 hours,
	-- if you want such a system please check out data/globalevents/globalevents.xml.
	globalSaveEnabled = false
	globalSaveHour = 8
	globalSaveMinute = 0
	shutdownAtGlobalSave = true
	cleanMapAtGlobalSave = false
	closeInstanceOnShutdown = true

	-- Spawns
	deSpawnRange = 2
	deSpawnRadius = 50
	allowBlockSpawn = true

	-- Summons
	maxPlayerSummons = 2
	teleportAllSummons = false
	teleportPlayerSummons = false

	-- Status
	ownerName = ""
	ownerEmail = "@otland.net"
	url = "http://otland.net/"
	location = "Europe"
	displayGamemastersWithOnlineCommand = false

	-- Logs
	displayPlayersLogging = true
	prefixChannelLogs = ""
	runFile = ""
	outputLog = ""
	errorLogName = ""
	truncateLogsOnStartup = false

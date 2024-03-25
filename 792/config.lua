	-- The Forgotten Server Config

	-- SQL Settings
	-- sqlType: mysql and sqlite supported.
	sqlType = "mysql"
	passwordType = "plain"
	mysqlHost = "127.0.0.1"
	mysqlUser = "root"
	mysqlPass = "root"
	mysqlPort = 3306
	mysqlDatabase = "nicaw"
	startupDatabaseOptimization = true
	sqliteDatabase = "forgottenserver.s3db"

	-- Connection Settings
	ip = "127.0.0.1"
	port = 7171
	loginTries = 0
	retryTimeout = 30 * 1000
	loginTimeout = 60 * 1000
	maxPlayers = 1000
	motd = "Welcome to the Forgotten Server!"
	onePlayerOnlinePerAccount = true
	allowClones = false
	serverName = "Forgotten"
	loginMessage = "Welcome to the Forgotten Server!"
	replaceKickOnLogin = true

	-- Account Manager
	accountManager = false
	newPlayerChooseVoc = true
	newPlayerSpawnPosX = 1000
	newPlayerSpawnPosY = 1000
	newPlayerSpawnPosZ = 7
	newPlayerTownId = 1
	newPlayerLevel = 8
	newPlayerMagicLevel = 0
	generateAccountNumber = false

	-- Skulls
	killsToRedSkull = 3
	timeToDecreaseFrags = 24 * 60 * 60 * 1000
	whiteSkullTime = 15 * 60 * 1000
	broadcastBanishments = true
	banDays = 7
	finalBanDays = 30
	killsToBan = 6

	-- Combat
	worldType = "pvp"
	hotkeyAimbotEnabled = true
	protectionLevel = 1
	pzLocked = 60000
	deathLosePercent = 10
	criticalHitChance = 7
	removeAmmoWhenUsingDistanceWeapon = true
	removeChargesFromRunes = true
	allowChangeOutfit = true
	noDamageToSameLookfeet = false
	experienceByKillingPlayers = false

	-- Highscores and Deathlist
	highscoreDisplayPlayers = 15
	updateHighscoresAfterMinutes = 60
	deathListEnabled = true
	maxDeathRecords = 10

	-- Houses
	-- Set housesPerOneAccount to -1 to disable this feature.
	housePriceEachSQM = 1000
	houseRentPeriod = "never"
	housesPerOneAccount = 1

	-- Item Usage Settings
	timeBetweenActions = 200
	timeBetweenExActions = 1000

	-- Map
	mapName = "Evolutions"
	mapAuthor = "Xidaozu"
	randomizeTiles = true
	mapStorageType = "relational"

	-- Premium Account
	freePremium = false

	-- Guilds
	ingameGuildSystem = true
	levelToCreateGuild = 8
	minGuildNameLength = 4
	maxGuildNameLength = 20

	-- Rates
	-- NOTE: experienceStages configuration is located in data/XML/stages.xml.
	experienceStages = false
	rateExperience = 5
	rateSkill = 3
	rateLoot = 2
	rateMagic = 3

	-- Monsters
	rateSpawn = 1
	deSpawnRange = 2
	deSpawnRadius = 50

	-- Stamina
	-- staminaOfflineGainAmount: is the amount of stamina that the player will recover according to the time he was offline
	-- staminaLostMonster: How many seconds of Stamina the player will lose for each monster killed
	staminaOfflineGainAmount = 10
	staminaLostMonster = 100

	-- Depot
	defaultDepotSizePremium = 2000
	defaultDepotSize = 1000

	-- VIP list
	vipListDefaultLimit = 20
	vipListDefaultPremiumLimit = 100

	-- Spells
	spellNameInsteadOfWords = false
	emoteSpells = false

	-- Real Server Save
	-- note: serverSaveHour means like 24:00, not that it will save every 24 hours,
	-- if you want such a system use autoSaveEachMinutes. this serversave method
	-- may be unstable, we recommend using otadmin if you want real serversaves.
	serverSaveEnabled = false
	serverSaveHour = 24
	shutdownAtServerSave = true

	-- Server saving
	autoSaveEachMinutes = 15
	saveGlobalStorage = false

	-- Clean System
	cleanProtectedZones = true
	cleanMapAtServerSave = true

	-- Startup
	displayOnOrOffAtCharlist = false
	defaultPriority = "high"

	-- Misc
	maxMessageBuffer = 4
	kickIdlePlayerAfterMinutes = 15
	enableRuleViolationReports = true
	stopAttackingAtExit = false

	-- Status
	ownerName = ""
	ownerEmail = "@otland.net"
	url = "http://otland.net/"
	location = "Europe"
	statusTimeout = 5 * 60 * 1000
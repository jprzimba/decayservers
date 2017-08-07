local keywordHandler = KeywordHandler:new()
local npcHandler = NpcHandler:new(keywordHandler)
NpcSystem.parseParameters(npcHandler)

function onCreatureAppear(cid)			npcHandler:onCreatureAppear(cid)			end
function onCreatureDisappear(cid)		npcHandler:onCreatureDisappear(cid)			end
function onCreatureSay(cid, type, msg)	npcHandler:onCreatureSay(cid, type, msg)	end
function onThink()						npcHandler:onThink()						end

function buyAddons(cid, message, keywords, parameters, node)
	--TODO: buyAddons function in modules.lua
	if(cid ~= npcHandler.focus) then
		return false
	end

	local addon = parameters.addon
	local cost = parameters.cost
	local premium = (parameters.premium ~= nil and parameters.premium ~= false)

	if isPlayerPremiumCallback == nil or isPlayerPremiumCallback(cid) == true or premium == false then
		if doPlayerRemoveMoney(cid, cost) == TRUE then
			doPlayerAddAddons(cid, addon)
			npcHandler:say('There, you are now able to use all addons!')
		else
			npcHandler:say('Sorry, you do not have enough money.')
		end
	else
		npcHandler:say('I only serve customers with premium accounts.')
	end

	keywordHandler:moveUp(1)
	return true
end

local nodeFirst = keywordHandler:addKeyword({'first addon'}, StdModule.say, {npcHandler = npcHandler, onlyFocus = true, text = 'Do you want to buy the first addons set for 5000 gold coins?.'})
	nodeFirst:addChildKeyword({'yes'}, buyAddons, {addon = 1, cost = 5000, premium = true})
	nodeFirst:addChildKeyword({'no'}, StdModule.say, {npcHandler = npcHandler, onlyFocus = true, moveup = 1, text = 'Too expensive, eh?'})

local nodeSecond = keywordHandler:addKeyword({'second addon'}, StdModule.say, {npcHandler = npcHandler, onlyFocus = true, text = 'Would you like to buy the second addons set for 10000 gold coins?.'})
	nodeSecond:addChildKeyword({'yes'}, buyAddons, {addon = 2, cost = 10000, premium = true})
	nodeSecond:addChildKeyword({'no'}, StdModule.say, {npcHandler = npcHandler, onlyFocus = true, moveup = 1, text = 'Too expensive, eh?'})

keywordHandler:addKeyword({'addon'}, StdModule.say, {npcHandler = npcHandler, onlyFocus = true, text = 'I sell the first addons set for 5000 gold coins and the second addons set for 10000 gold coins.'})

npcHandler:addModule(FocusModule:new())
local keywordHandler = KeywordHandler:new()
local npcHandler = NpcHandler:new(keywordHandler)
NpcSystem.parseParameters(npcHandler)

function onCreatureAppear(cid)		npcHandler:onCreatureAppear(cid)		end
function onCreatureDisappear(cid)	npcHandler:onCreatureDisappear(cid)		end
function onCreatureSay(cid, type, msg)	npcHandler:onCreatureSay(cid, type, msg)	end
function onThink()			npcHandler:onThink()				end

local shopModule = ShopModule:new()
npcHandler:addModule(shopModule)

shopModule:addBuyableItem({'light wand', 'lightwand'}, 		2163, 500, 		'magic light wand')
shopModule:addBuyableItem({'mana fluid', 'manafluid'}, 		2006, 55, 	7, 	'mana fluid')
shopModule:addBuyableItem({'life fluid', 'lifefluid'}, 		2006, 50, 	10,	'life fluid')
shopModule:addBuyableItem({'blank'}, 				2260, 10, 		'blank rune')

shopModule:addBuyableItem({'wand of inferno', 'inferno'}, 				2187, 15000, 	'wand of inferno')
shopModule:addBuyableItem({'wand of plague', 'plague'}, 				2188, 5000, 	'wand of plague')
shopModule:addBuyableItem({'wand of cosmic energy', 'cosmic energy'}, 			2189, 10000,	'wand of cosmic energy')
shopModule:addBuyableItem({'wand of vortex', 'vortex'}, 				2190, 500, 	'wand of vortex')
shopModule:addBuyableItem({'wand of dragonbreath', 'dragonbreath'}, 			2191, 1000, 	'wand of dragonbreath')

shopModule:addBuyableItem({'quagmire rod', 'quagmire'}, 				2181, 10000, 	'quagmire rod')
shopModule:addBuyableItem({'snakebite rod', 'snakebite'}, 				2182, 500, 	'snakebite rod')
shopModule:addBuyableItem({'tempest rod', 'tempest'}, 					2183, 15000, 	'tempest rod')
shopModule:addBuyableItem({'volcanic rod', 'volcanic'}, 				2185, 5000, 	'volcanic rod')
shopModule:addBuyableItem({'moonlight rod', 'moonlight'}, 				2186, 1000,   	'moonlight rod')

function creatureSayCallback(cid, type, msg)
	if(npcHandler.focus ~= cid) then
		return FALSE
	end
	if msgcontains(msg, 'runes') then
		selfSay("I sell heavy magic missiles runes, explosion runes, great fireball runes, ultimate healing runes, sudden death runes, mana runes and blank runes.")
		talk_state = 0
	elseif msgcontains(msg, 'heavy magic missile') or msgcontains(msg, 'hmm') and ShopModule:getCount(msg) <= 100 then
		charges = ShopModule:getCount(msg)
		price = 15*charges
		selfSay('Do you want a heavy magic missile rune with '..charges..' charges for '..price..' gold coins?')
		talk_state = 1
		itemid = 2311
	elseif msgcontains(msg, 'great fireball') or msgcontains(msg, 'gfb') and ShopModule:getCount(msg) <= 100 then
		charges = ShopModule:getCount(msg)
		price = 25*charges
		selfSay('Do you want a great fireball rune with '..charges..' charges for '..price..' gold coins?')
		talk_state = 1
		itemid = 2304
	elseif msgcontains(msg, 'explosion') or msgcontains(msg, 'xpl') and ShopModule:getCount(msg) <= 100 then
		charges = ShopModule:getCount(msg)
		price = 40*charges
		selfSay('Do you want an explosion rune with '..charges..' charges for '..price..' gold coins?')
		talk_state = 1
		itemid = 2313
	elseif msgcontains(msg, 'ultimate healing') or msgcontains(msg, 'uh') and ShopModule:getCount(msg) <= 100 then
		charges = ShopModule:getCount(msg)
		price = 35*charges
		selfSay('Do you want an ultimate healing rune with '..charges..' charges for '..price..' gold coins?')
		talk_state = 1
		itemid = 2273
	elseif msgcontains(msg, 'sudden death') or msgcontains(msg, 'sd') and ShopModule:getCount(msg) <= 100 then
		charges = ShopModule:getCount(msg)
		price = 50*charges
		selfSay('Do you want a sudden death rune with '..charges..' charges for '..price..' gold coins?')
		talk_state = 1
		itemid = 2268
	elseif msgcontains(msg, 'mana') and ShopModule:getCount(msg) <= 100 then
		charges = ShopModule:getCount(msg)
		price = 50*charges
		selfSay('Do you want a mana rune with '..charges..' charges for '..price..' gold coins?')
		talk_state = 1
		itemid = 2284
	elseif msgcontains(msg, 'yes') and talk_state == 1 then
		if doPlayerRemoveMoney(cid, price) == TRUE then
			doPlayerGiveItem(cid, itemid, 1, charges)
			selfSay("You have bought this rune.")
			talk_state = 0
		else
			selfSay("You don't have enough money.")
			talk_state = 0
		end
	elseif msgcontains(msg, 'no') and talk_state == 1 then
		selfSay("Then not.")
		talk_state = 0
	end
	return TRUE
end

npcHandler:setCallback(CALLBACK_MESSAGE_DEFAULT, creatureSayCallback)
npcHandler:addModule(FocusModule:new())
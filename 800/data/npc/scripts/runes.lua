local keywordHandler = KeywordHandler:new()
local npcHandler = NpcHandler:new(keywordHandler)
NpcSystem.parseParameters(npcHandler)



-- OTServ event handling functions start
function onCreatureAppear(cid)				npcHandler:onCreatureAppear(cid) end
function onCreatureDisappear(cid) 			npcHandler:onCreatureDisappear(cid) end
function onCreatureSay(cid, type, msg) 	npcHandler:onCreatureSay(cid, type, msg) end
function onThink() 						npcHandler:onThink() end
-- OTServ event handling functions end

local shopModule = ShopModule:new()
npcHandler:addModule(shopModule)

shopModule:addBuyableItem({'light wand', 'lightwand'}, 					2162, 500, 		'magic light wand')
shopModule:addBuyableItem({'mana fluid', 'manafluid', 'mf'}, 					2006, 50, 	7, 	'mana fluid')
shopModule:addBuyableItem({'life fluid', 'lifefluid'}, 					2006, 80, 	10,	'life fluid')

shopModule:addBuyableItem({'heavy magic missile', 'hmm'}, 				2311, 125, 	10,	'heavy magic missile rune')
shopModule:addBuyableItem({'great fireball', 'gfb'}, 					2304, 180, 	4, 	'great fireball rune')
shopModule:addBuyableItem({'explo', 'xpl'}, 							2313, 250, 	6, 	'explosion rune')
shopModule:addBuyableItem({'ultimate healing', 'uh'}, 					2273, 175, 	2, 	'ultimate healing rune')
shopModule:addBuyableItem({'sudden death', 'sd'}, 						2268, 325, 	2, 	'sudden death rune')
shopModule:addBuyableItem({'blank', 'blankrune', 'blank'}, 							2260, 10, 		'blank rune')

shopModule:addBuyableItem({'wand of inferno', 'inferno'}, 				2187, 15000, 	'wand of inferno')
shopModule:addBuyableItem({'wand of plague', 'plague'}, 				2188, 5000, 	'wand of plague')
shopModule:addBuyableItem({'wand of cosmic energy', 'cosmic energy'}, 	2189, 10000, 	'explosion rune')
shopModule:addBuyableItem({'wand of vortex', 'vortex'}, 				2190, 500, 	 	'wand of cosmic energy')
shopModule:addBuyableItem({'wand of dragonbreath', 'dragonbreath'}, 	2191, 1000, 	'wand of dragonbreath')

shopModule:addBuyableItem({'quagmire rod', 'quagmire'}, 				2181, 10000, 	'quagmire rod')
shopModule:addBuyableItem({'snakebite rod', 'snakebite'}, 				2182, 500, 	 	'snakebite rod')
shopModule:addBuyableItem({'tempest rod', 'tempest'}, 					2183, 15000, 	'tempest rod')
shopModule:addBuyableItem({'volcanic rod', 'volcanic'}, 				2185, 5000, 	'volcanic rod')
shopModule:addBuyableItem({'moonlight rod', 'moonlight'}, 				2186, 1000,   	'moonlight rod')

npcHandler:addModule(FocusModule:new())
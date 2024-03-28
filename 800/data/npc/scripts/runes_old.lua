local keywordHandler = KeywordHandler:new()
local npcHandler = NpcHandler:new(keywordHandler)
NpcSystem.parseParameters(npcHandler)

function onCreatureAppear(cid)			npcHandler:onCreatureAppear(cid)			end
function onCreatureDisappear(cid)		npcHandler:onCreatureDisappear(cid)			end
function onCreatureSay(cid, type, msg)	npcHandler:onCreatureSay(cid, type, msg)	end
function onThink()						npcHandler:onThink()						end

function creatureSayCallback(cid, type, msg)
	--- TODO: bless function in modules.lua
	if(npcHandler.focus ~= cid) then
		return FALSE
	end

	if msgcontains(msg, 'job') then
		npcHandler:say('I am the head alchemist of this City. I keep the secret recipies of our ancestors. Besides, I am selling mana and life fluids,spellbooks, wands, rods and runes.')
		talkState = 0


-- Main Runes ---------------------------------------------------------------------------------

		elseif msgcontains(msg, 'spell rune') then
			npcHandler:say('I sell missile runes, explosive runes, field runes, wall runes, bomb runes, healing runes, convince creature rune and chameleon rune.')

		elseif msgcontains(msg, 'missile runes') then
			npcHandler:say('I can offer you light magic missile runes, heavy magic missile runes and sudden death runes.')

		elseif msgcontains(msg, 'explosive runes') then
			npcHandler:say('I can offer you fireball runes, great fireball runes and explosion runes.')

		elseif msgcontains(msg, 'field runes') then
			npcHandler:say('I can offer you fire field runes, energy field runes, poison field runes and destroy field runes.')

		elseif msgcontains(msg, 'wall runes') then
			npcHandler:say('I can offer you fire wall runes, energy wall runes and poison wall runes.')

		elseif msgcontains(msg, 'bomb runes') then
			npcHandler:say('I can offer you firebomb runes.')

		elseif msgcontains(msg, 'healing runes') then
			npcHandler:say('I can offer you antidote runes, intense healing runes and ultimate healing runes.')

-- Runes ---------------------------------------------------------------------------------

		elseif msgcontains(msg, 'light magic missile') then
			count = getCount(msg)
			cost = count*10
			talkState = 20
			if count >= 2 then
			    npcHandler:say('Do you want to buy ' .. count .. ' light magic missile runes for ' .. cost .. '.')
			else
			    npcHandler:say('Do you want to buy a light magic missile rune for 10 gold?')
			end

		elseif msgcontains(msg, 'heavy magic missile') then
			count = getCount(msg)
			cost = count*20
			talkState = 21
			if count >= 2 then
			    npcHandler:say('Do you want to buy ' .. count .. ' heavy magic missile runes for ' .. cost .. '.')
			else
			    npcHandler:say('Do you want to buy a heavy magic missile rune for 20 gold?')
			end

		elseif msgcontains(msg, 'sudden death') then
			count = getCount(msg)
			cost = count*230
			talkState = 22
			if count >= 2 then
			    npcHandler:say('Do you want to buy ' .. count .. ' sudden death runes for ' .. cost .. '.')
			else
			    npcHandler:say('Do you want to buy a sudden death rune for 230 gold?')
			end

		elseif msgcontains(msg, 'great fireball') then
			count = getCount(msg)
			cost = count*45
			talkState = 23
			if count >= 2 then
			    npcHandler:say('Do you want to buy ' .. count .. ' great fireball runes for ' .. cost .. '.')
			else
			    npcHandler:say('Do you want to buy a great fireball rune for 45 gold?')
			end

		elseif msgcontains(msg, 'explosion') then
			count = getCount(msg)
			cost = count*60
			talkState = 24
			if count >= 2 then
			npcHandler:say('Do you want to buy ' .. count .. ' explosion runes for ' .. cost .. '.')
			else
			    npcHandler:say('Do you want to buy a explosion rune for 60 gold?')
			end

		elseif msgcontains(msg, 'fire field') then
			count = getCount(msg)
			cost = count*20
			talkState = 25
			if count >= 2 then
			npcHandler:say('Do you want to buy ' .. count .. ' fire field runes for ' .. cost .. '.')
			else
			    npcHandler:say('Do you want to buy a fire field rune for 20 gold?')
			end

		elseif msgcontains(msg, 'energy field') then
			count = getCount(msg)
			cost = count*30
			talkState = 26
			if count >= 2 then
			npcHandler:say('Do you want to buy ' .. count .. ' energy field runes for ' .. cost .. '.')
			else
			    npcHandler:say('Do you want to buy a energy field rune for 30 gold?')
			end

		elseif msgcontains(msg, 'poison field') then
			count = getCount(msg)
			cost = count*15
			talkState = 27
			if count >= 2 then
			npcHandler:say('Do you want to buy ' .. count .. ' poison field runes for ' .. cost .. '.')
			else
			    npcHandler:say('Do you want to buy a poison field rune for 15 gold?')
			end

		elseif msgcontains(msg, 'destroy field') then
			count = getCount(msg)
			cost = count*20
			talkState = 28
			if count >= 2 then
			npcHandler:say('Do you want to buy ' .. count .. ' destroy field runes for ' .. cost .. '.')
			else
			    npcHandler:say('Do you want to buy a destroy field rune for 20 gold?')
			end

		elseif msgcontains(msg, 'fire wall') then
			count = getCount(msg)
			cost = count*45
			talkState = 29
			if count >= 2 then
			npcHandler:say('Do you want to buy ' .. count .. ' fire wall runes for ' .. cost .. '.')
			else
			    npcHandler:say('Do you want to buy a fire wall rune for 45 gold?')
			end

		elseif msgcontains(msg, 'energy wall') then
			count = getCount(msg)
			cost = count*60
			talkState = 30
			if count >= 2 then
			npcHandler:say('Do you want to buy ' .. count .. ' energy wall runes for ' .. cost .. '.')
			else
			    npcHandler:say('Do you want to buy a energy wall rune for 60 gold?')
			end

		elseif msgcontains(msg, 'poison wall') then
			count = getCount(msg)
			cost = count*35
			talkState = 31
			if count >= 2 then
			npcHandler:say('Do you want to buy ' .. count .. ' poison wall runes for ' .. cost .. '.')
			else
			    npcHandler:say('Do you want to buy a poison wall rune for 35 gold?')
			end

		elseif msgcontains(msg, 'antidote') then
			count = getCount(msg)
			cost = count*45
			talkState = 32
			if count >= 2 then
			npcHandler:say('Do you want to buy ' .. count .. ' antidote runes for ' .. cost .. '.')
			else
			    npcHandler:say('Do you want to buy a antidote rune for 45 gold?')
			end

		elseif msgcontains(msg, 'intense healing') then
			count = getCount(msg)
			cost = count*65
			talkState = 33
			if count >= 2 then
			npcHandler:say('Do you want to buy ' .. count .. ' intense healing runes for ' .. cost .. '.')
			else
			    npcHandler:say('Do you want to buy a intense healing rune for 65 gold?')
			end

		elseif msgcontains(msg, 'ultimate healing') then
			count = getCount(msg)
			cost = count*130
			talkState = 34
			if count >= 2 then
			npcHandler:say('Do you want to buy ' .. count .. ' ultimate healing runes for ' .. cost .. '.')
			else
			    npcHandler:say('Do you want to buy a ultimate healing rune for 130 gold?')
			end

		elseif msgcontains(msg, 'blank') then
			count = getCount(msg)
			cost = count*10
			talkState = 35
			if count >= 2 then
			npcHandler:say('Do you want to buy ' .. count .. ' blank runes for ' .. cost .. '.')
			else
			    npcHandler:say('Do you want to buy a blank rune for 10 gold?')
			end

-- Wands and Rods --------------------------------------------------------------------------
	
		elseif msgcontains(msg, 'Wand of Vortex') then
			talkState = 50
			npcHandler:say('This wand is only for sorcerers of level 7 and above. Would you like to buy a wand of vortex for 500 gold?')

		elseif msgcontains(msg, 'Wand of Dragonbreath') then
			talkState = 51
			npcHandler:say('This wand is only for sorcerers of level 13 and above. Would you like to buy a wand of dragonbreath for 1000 gold?')

		elseif msgcontains(msg, 'Wand of Plague') then
			talkState = 52
			npcHandler:say('This wand is only for sorcerers of level 19 and above. Would you like to buy a wand of plague for 5000 gold?')

		elseif msgcontains(msg, 'Wand of Cosmic Energy') then
			talkState = 53
			npcHandler:say('This wand is only for sorcerers of level 26 and above. Would you like to buy a wand of cosmic energy for 10000 gold?')

		elseif msgcontains(msg, 'Wand of Inferno') then
			npcHandler:say('Sorry, this wand contains magic far too powerful and we are afraid to store it here. I heard they have a few of these at the Premium academy though.')

		elseif msgcontains(msg, 'Snakebite Rod') then
			talkState = 55
			npcHandler:say('This rod is only for druids of level 7 and above. Would you like to buy a snakebite rod for 500 gold?')

		elseif msgcontains(msg, 'Moonlight Rod') then
			talkState = 56
			npcHandler:say('This rod is only for druids of level 13 and above. Would you like to buy a moonlight rod for 1000 gold?')

		elseif msgcontains(msg, 'Volcanic Rod') then
			talkState = 57
			npcHandler:say('This rod is only for druids of level 19 and above. Would you like to buy a volcanic rod for 5000 gold?')

		elseif msgcontains(msg, 'Quagmire Rod') then
			talkState = 58
			npcHandler:say('This rod is only for druids of level 26 and above. Would you like to buy a quagmire rod for 10000 gold?')

		elseif msgcontains(msg, 'Tempest Rod') then
			npcHandler:say('Sorry, this rod contains magic far too powerful and we are afraid to store it here. I heard they have a few of these at the Premium academy though.')
	
		elseif msgcontains(msg, 'wand') or msgcontains(msg, 'wands') then
			npcHandler:say('Wands can be wielded by sorcerers only and have a certain level requirement. There are five different wands, would you like to hear about them?')
			talkState = 2
			
		elseif msgcontains(msg, 'rod') or msgcontains(msg, 'rods') then
			npcHandler:say('Rods can be wielded by druids only and have a certain level requirement. There are five different rods, would you like to hear about them?')
			talkState = 3

-- Others --------------------------------------------------------------------------------

		elseif msgcontains(msg, 'mana fluid') or msgcontains(msg, 'mana fluids') or msgcontains(msg, 'mf') then
			count = getCount(msg)
			cost = count*55
			talkState = 36
			if count >= 2 then
			    npcHandler:say('Do you want to buy ' .. count .. ' mana fluids for ' .. cost .. '.')
			else
			     npcHandler:say('Do you want to buy mana fluid for 55 gold')
			end

		elseif msgcontains(msg, 'life fluid') or msgcontains(msg, 'life fluids') then
		       	count = getCount(msg)
			cost = count*60
			talkState = 37
			if count >= 2 then
			    npcHandler:say('Do you want to buy ' .. count .. ' life fluids for ' .. cost .. '.')
			else
			     npcHandler:say('Do you want to buy life fluid for 60 gold')
			end

		elseif msgcontains(msg, 'spellbook') then
		       talkState = 38
		       npcHandler:say('A spellbook is a nice tool for beginners. Do you want to buy one for 150 gold?')	

-- backpacks of runes ------------------------------------------------------------------
		elseif msgcontains(msg, 'bpmana') or msgcontains(msg, 'manabp') then
			cost = 1100
			talkState = 100
			npcHandler:say('Do you want to buy bp of mana fluid for ' .. cost  .. ' gold')

	elseif talkState > 0 then
		if msgcontains(msg, 'yes') then
			if talkState == 2 then
				npcHandler:say('The names of the wands are \'Wand of Vortex\', \'Wand of Dragonbreath\', \'Wand of Plague\', \'Wand of Cosmic Energy\' and \'Wand ofInferno\'. Which one would you like to buy?')
			elseif talkState == 3 then
				npcHandler:say('The names of the rods are \'Snakebite Rod\', \'Moonlight Rod\', \'Volcanic Rod\', \'Quagmire Rod\', and \'Tempest Rod\'. Which one would you like to buy?')
			elseif talkState == 20 then
                                buy(cid,2287,count,10)
			elseif talkState == 21 then
				buy(cid,2311,count,20)
			elseif talkState == 22 then
				buy(cid,2268,count,230)
			elseif talkState == 23 then
                                buy(cid,2304,count,45)
			elseif talkState == 24 then
				buy(cid,2313,count,60)	
			elseif talkState == 25 then
				buy(cid,2301,count,20)
			elseif talkState == 26 then
                                buy(cid,2277,count,30)
			elseif talkState == 27 then
				buy(cid,2285,count,15)	
			elseif talkState == 28 then
				buy(cid,2261,count,20)
			elseif talkState == 29 then
                                buy(cid,2303,count,45)
			elseif talkState == 30 then
				buy(cid,2279,count,60)	
			elseif talkState == 31 then
				buy(cid,2289,count,35)
			elseif talkState == 32 then
                                buy(cid,2266,count,45)
			elseif talkState == 33 then
				buy(cid,2265,count,65)
			elseif talkState == 34 then
				buy(cid,2273,count,130)
			elseif talkState == 35 then
				buy(cid,2260,count,10)
			elseif talkState == 36 then
				buyFluidContainer(cid,2006,count,55,7)
			elseif talkState == 37 then
				buyFluidContainer(cid,2006,count,60,10)
			elseif talkState == 38 then
				buy(cid,2175,1,150)
			elseif talkState == 50 then
				buy(cid,2190,1,500)
			elseif talkState == 51 then
				buy(cid,2191,1,1000)
			elseif talkState == 52 then
				buy(cid,2188,1,5000)
			elseif talkState == 53 then
				buy(cid,2189,1,10000)
			elseif talkState == 55 then
				buy(cid,2182,1,500)
			elseif talkState == 56 then
				buy(cid,2186,1,1000)
			elseif talkState == 37 then
				buy(cid,2185,1,5000)
			elseif talkState == 58 then
				buy(cid,2181,1,10000)	

-- backpack of runes -------------------------------------------------------------------------------------------------------------
			elseif talkState == 100 then
				buyContainer(cid, 2001, 2006, 7, cost)
			end
		elseif msgcontains(msg, 'no') then
			npcHandler:say('Good bye.')
		end
		talkState = 0
	end
	return TRUE
end

npcHandler:setCallback(CALLBACK_MESSAGE_DEFAULT, creatureSayCallback)
npcHandler:addModule(FocusModule:new())
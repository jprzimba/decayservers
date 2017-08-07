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

	if msgcontains(msg, 'blessing') or msgcontains(msg, 'blessings') or msgcontains(msg, 'help') or msgcontains(msg, 'offer') then
		selfSay("I can provide you with five blessings... the 'first bless', 'second bless', 'third bless', 'fourth bless' and the 'fifth bless', they cost 10000 gold coins each.")
		talkState = 0
	elseif msgcontains(msg, 'first bless') then
		selfSay("Do you want to buy the first blessing for 10000 gold?")
		talkState = 1
	elseif msgcontains(msg, 'second bless') then
		selfSay("Do you want to buy the second blessing for 10000 gold?")
		talkState = 2
	elseif msgcontains(msg, 'third bless') then
		selfSay("Do you want to buy the third blessing for 10000 gold?")
		talkState = 3
	elseif msgcontains(msg, 'fourth bless') then
		selfSay("Do you want to buy the fourth blessing for 10000 gold?")
		talkState = 4
	elseif msgcontains(msg, 'fifth bless') then
		selfSay("Do you want to buy the fifth blessing for 10000 gold?")
		talkState = 5
	elseif talkState > 0 then
		if msgcontains(msg, 'yes') then
			if getPlayerBlessing(cid, talkState) then
				selfSay("A god has already blessed you with this blessing.")
			elseif isPremium(cid) == TRUE then
				if doPlayerRemoveMoney(cid, 10000) == TRUE then
					doPlayerAddBlessing(cid, talkState)
					selfSay("You have been blessed by one of the five gods!")
				else
					selfSay("You don't have enough money.")
				end
			else
				selfSay("You need a premium account to buy blessings.")
			end
		elseif msgcontains(msg, 'no') then
			selfSay("Then not.")
		end
		talk_state = 0
	end
	return TRUE
end

npcHandler:setCallback(CALLBACK_MESSAGE_DEFAULT, creatureSayCallback)
npcHandler:addModule(FocusModule:new())
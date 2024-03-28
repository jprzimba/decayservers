local keywordHandler = KeywordHandler:new()
local npcHandler = NpcHandler:new(keywordHandler)
NpcSystem.parseParameters(npcHandler)
local talkState = 0

function onCreatureAppear(cid)				npcHandler:onCreatureAppear(cid)			end
function onCreatureDisappear(cid)			npcHandler:onCreatureDisappear(cid)			end
function onCreatureSay(cid, type, msg)			npcHandler:onCreatureSay(cid, type, msg)		end
function onThink()					npcHandler:onThink()					end

function creatureSayCallback(cid, type, msg)
	if(cid ~= npcHandler.focus) then
		return false
	end

	if(msgcontains(msg, 'soft') or msgcontains(msg, 'boots')) then
		npcHandler:say('Do you want to repair your worn soft boots for 10000 gold coins?')
		talkState = 1
	elseif(msgcontains(msg, 'yes') and talkState == 1) then
		if(getPlayerItemCount(cid, 6530) >= 1) then
			if(doPlayerRemoveMoney(cid, 10000)) then
				local item = getPlayerItemById(cid, true, 6530)
				doTransformItem(item.uid, 6132)
				npcHandler:say('Here you are.')
			else
				npcHandler:say('Sorry, you don\'t have enough gold.')
			end
		elseif(getPlayerItemCount(cid, 10021) >= 1) then
			if(doPlayerRemoveMoney(cid, 10000)) then
				local item = getPlayerItemById(cid, true, 10021)
				doTransformItem(item.uid, 6132)
				npcHandler:say('Here you are.')
			else
				npcHandler:say('Sorry, you don\'t have enough gold.')
			end
		else
			npcHandler:say('Sorry, you don\'t have the item.')
		end
		talkState = 0
	elseif(msgcontains(msg, 'no')) then
		talkState = 0
		npcHandler:say('Ok then.')
	end

	return true
end

npcHandler:setCallback(CALLBACK_MESSAGE_DEFAULT, creatureSayCallback)
npcHandler:addModule(FocusModule:new())

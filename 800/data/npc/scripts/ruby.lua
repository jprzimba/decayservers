local keywordHandler = KeywordHandler:new()
local npcHandler = NpcHandler:new(keywordHandler)
NpcSystem.parseParameters(npcHandler)

function onCreatureAppear(cid)			npcHandler:onCreatureAppear(cid)			end
function onCreatureDisappear(cid)		npcHandler:onCreatureDisappear(cid)			end
function onCreatureSay(cid, type, msg)	npcHandler:onCreatureSay(cid, type, msg)	end
function onThink()						npcHandler:onThink()						end

function creatureSayCallback(cid, type, msg)
	if(cid ~= npcHandler.focus) then
		return false
	end

	npcHandler:say("Take this!")
	doTargetCombatHealth(getNpcCid(), cid, COMBAT_LIFEDRAIN, -100, -200, CONST_ME_BLOCKHIT)
end

npcHandler:setCallback(CALLBACK_MESSAGE_DEFAULT, creatureSayCallback)
npcHandler:addModule(FocusModule:new())
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

	if msgcontains(msg, 'manage') or msgcontains(msg, 'help') then
		npcHandler:say("You can use the following commands to create or manage your guild: !createguild guildName, !joinguild guildName, !invite playerName, !disband")
		talkState = 0
	end
	return TRUE
end

npcHandler:setCallback(CALLBACK_MESSAGE_DEFAULT, creatureSayCallback)
npcHandler:addModule(FocusModule:new())
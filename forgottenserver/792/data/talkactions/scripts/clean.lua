local cleanEvent = 0

function onSay(cid, words, param)
	if(param == '') then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Collected " .. doCleanMap() .. " items.")
		return false
	end

	if(not tonumber(param)) then
		doPlayerSendCancel(cid, "Command requires numeric param.")
		return false
	end

	stopEvent(cleanEvent)
	prepareClean(tonumber(param), cid)
	return false
end

function prepareClean(minutes, cid)
	if(minutes == 0) then
		if(isPlayer(cid)) then
			doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Cleaned " .. doCleanMap() .. " items.")
		end

		broadcastMessage("Game map cleaned.", MESSAGE_STATUS_WARNING)
	elseif(minutes > 0) then
		if(minutes == 1) then
			broadcastMessage("Game map cleaning in " .. minutes .. " minute, please pick up all your items.", MESSAGE_STATUS_WARNING)
		else
			broadcastMessage("Game map cleaning in " .. minutes .. " minutes.", MESSAGE_STATUS_WARNING)
		end

		cleanEvent = addEvent(prepareClean, 60000, minutes - 1, cid)
	end
end

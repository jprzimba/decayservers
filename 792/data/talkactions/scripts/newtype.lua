local function checkType(value)
	return not(value <= 1 or value == 135 or (value > 160 and value < 192) or value < 247)
end

function onSay(cid, words, param)
	if(param == '') then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Command param required.")
		return true
	end

	local t = string.explode(param, ",")
	t[1] = tonumber(t[1])
	if(not t[1]) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Command requires numeric param.")
		return true
	end

	local pid = cid
	if(t[2]) then
		pid = getPlayerByNameWildcard(t[2])
		if(not pid or (isPlayerGhost(pid))) then
			doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Player " .. t[2] .. " not found.")
			return true
		end
	end

	if(t[1] <= 1 or t[1] == 135 or (t[1] > 160 and t[1] < 192) or t[1] > 247) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Such outfit does not exist.")
		return false
	end

	local tmp = getCreatureOutfit(pid)
	tmp.lookType = t[1]

	doCreatureChangeOutfit(pid, tmp)
	return false
end

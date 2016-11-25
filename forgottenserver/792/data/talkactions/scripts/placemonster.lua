function onSay(cid, words, param)
	local func = doCreateMonster
	local pid = cid
	local t = string.explode(param, " ")
	if(t[2]) then
		pid = getPlayerByNameWildcard(t[2])
		if(not pid) then
			doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Player " .. t[2] .. " not found.")
			return false
		end
	end

	local position = getCreaturePosition(pid)
	local effect = CONST_ME_MAGIC_RED
	local ret = func(t[1], position, false)
	if(tonumber(ret) == nil) then
		effect = CONST_ME_POFF
		doPlayerSendDefaultCancel(cid, (ret == false and RETURNVALUE_NOTPOSSIBLE or RETURNVALUE_NOTENOUGHROOM))
	end

	doSendMagicEffect(position, effect)
	return false
end

function onSay(cid, words, param)
	if (param == '') then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Command requires param.")
		return false
	end

	local creature = getCreatureByName(param)
	local player = getPlayerByNameWildcard(param)
	local tile = string.explode(param, ",")
	local pos = {x = 0, y = 0, z = 0}

	if (player ~= nil and not isPlayerGhost(player)) then
		pos = getCreaturePosition(player)
	elseif (creature ~= nil and (not isPlayer(creature) or (not isPlayerGhost(creature)))) then
		pos = getCreaturePosition(creature)
	elseif (tile[2] and tile[3]) then
		pos = {x = tile[1], y = tile[2], z = tile[3]}
	else
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Invalid param specified.")
		return false
	end

	if (not pos or isInArray({pos.x, pos.y}, 0)) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Destination not reachable.")
		return false
	end

	pos = getClosestFreeTile(cid, pos, true, false)
	if (not pos or isInArray({pos.x, pos.y}, 0)) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Cannot perform action.")
		return false
	end

	local tmp = getCreaturePosition(cid)
	if (doTeleportThing(cid, pos, true) and not isPlayerGhost(cid)) then
		doSendMagicEffect(tmp, CONST_ME_POFF)
		doSendMagicEffect(pos, CONST_ME_TELEPORT)
	end

	return false
end

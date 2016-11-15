function isPlayerPremiumCallback(cid)
	if getPlayerPremiumDays(cid) > 0 then
		return true
	end
	return false
end

function msgcontains(message, keyword)
	local a, b = string.find(message, keyword)
	if a == nil or b == nil then
		return false
	end
	return true
end

function getDistanceToCreature(cid)
	if isPlayer(cid) == FALSE or cid == 0 or cid == nil then
		return FALSE
	end
	local creaturePos = getCreaturePosition(cid)
	local cx = creaturePos.x
	local cy = creaturePos.y
	local cz = creaturePos.z
	local sx, sy, sz = selfGetPosition()
	if cx == nil or cz ~= sz then
		return 100
	end
	return math.max(math.abs(sx - cx), math.abs(sy - cy))
end

function doPosRemoveItem(_itemid, n, position)
	local thing = getThingfromPos({x = position.x, y = position.y, z = position.z, stackpos = 1})
	if thing.itemid == _itemid then
		doRemoveItem(thing.uid, n)
	else
		return false
	end
	return true
end

-- Including the Advanced NPC System
dofile('data/npc/lib/npcsystem/npcsystem.lua')
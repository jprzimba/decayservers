local spotId = {384, 418}
local holeId = {
	294, 369, 370, 383, 392,
	408, 409, 427, 428, 430,
	462, 469, 470, 482, 484,
	485, 489, 924, 3135, 3136
}

function onUse(cid, item, fromPosition, itemEx, toPosition)
	if(toPosition.x == CONTAINER_POSITION) then
		doPlayerSendDefaultCancel(cid, RETURNVALUE_NOTPOSSIBLE)
		return true
	end

	local itemGround = getThingFromPos(toPosition)
	if(isInArray(spotId, itemGround.itemid)) then
		doTeleportThing(cid, {x = toPosition.x, y = toPosition.y + 1, z = toPosition.z - 1}, false)
	elseif(isInArray(holeId, itemEx.itemid)) then
		local hole = getThingFromPos({x = toPosition.x, y = toPosition.y, z = toPosition.z + 1, stackpos = STACKPOS_TOP_MOVEABLE_ITEM_OR_CREATURE})
		if(hole.itemid > 0) then
			doTeleportThing(hole.uid, {x = toPosition.x, y = toPosition.y + 1, z = toPosition.z}, false)
		else
			doPlayerSendDefaultCancel(cid, RETURNVALUE_NOTPOSSIBLE)
		end
	else
		return false
	end

	return true
end

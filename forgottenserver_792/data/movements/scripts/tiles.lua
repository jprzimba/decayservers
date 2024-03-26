local increasingItemID = {416, 446, 3216}
local decreasingItemID = {417, 447, 3217}
function onStepIn(cid, item, position, fromPosition)
	if isInArray(increasingItemID, item.itemid) == TRUE then
		doTransformItem(item.uid, item.itemid + 1)
		if item.actionid > 1000 then
			getLevelTile(cid, item, position)
		elseif getTilePzInfo(position) == TRUE then
			getDepotItems(cid, item)
		end
	elseif item.itemid == 426 then
		doTransformItem(item.uid, 425)
		if item.actionid > 1000 then
			getLevelTile(cid, item, position)
		elseif getTilePzInfo(position) == TRUE then
			getDepotItems(cid, item)
		end
	end
	return TRUE
end

function onStepOut(cid, item, position, fromPosition)
	if isInArray(decreasingItemID, item.itemid) == TRUE then
		doTransformItem(item.uid, item.itemid - 1)
	elseif item.itemid == 425 then
		doTransformItem(item.uid, item.itemid + 1)
	end
	return TRUE
end

function getLevelTile(cid, item, position)
	if isPlayer(cid) == TRUE then
		if getPlayerLevel(cid) < item.actionid - 1000 then
			doTeleportThing(cid, {x = getPlayerPosition(cid).x, y = getPlayerPosition(cid).y, z = getPlayerPosition(cid).z + 1}, FALSE)
			doSendMagicEffect(position, CONST_ME_MAGIC_BLUE)
		end
	end
	return TRUE
end

function getDepotItems(cid, item)
	if item.actionid > 100 then
		if isPlayer(cid) == TRUE then
			depotItems = getPlayerDepotItems(cid, item.actionid - 100)
			if depotItems < 2 then
				doPlayerSendTextMessage(cid, MESSAGE_EVENT_DEFAULT, "Your depot contains 1 item.")
			else
				doPlayerSendTextMessage(cid, MESSAGE_EVENT_DEFAULT, "Your depot contains " ..depotItems.. " items.")
			end
		end
	end
	return TRUE
end
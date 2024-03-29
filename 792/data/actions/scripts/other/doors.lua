function onUse(cid, item, fromPosition, itemEx, toPosition)
    if isInArray(questDoors, item.itemid) then
        if getPlayerStorageValue(cid, item.actionid) ~= -1 then
            doTransformItem(item.uid, item.itemid + 1)
            doTeleportThing(cid, toPosition, true)
        else
            doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "The door seems to be sealed against unwanted intruders.")
        end
        return true
	elseif isInArray(levelDoors, item.itemid) then
		local requiredLevel = item.actionid - 1000
		if item.actionid > 0 and getPlayerLevel(cid) >= requiredLevel then
			doTransformItem(item.uid, item.itemid + 1)
			doTeleportThing(cid, toPosition, true)
		else
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You need level " .. requiredLevel .. " to pass through this door.")
		end
		return true	
    elseif isInArray(keys, item.itemid) then
        if itemEx.actionid > 0 then
            if item.actionid == itemEx.actionid then
                if doors[itemEx.itemid] ~= nil then
                    doTransformItem(itemEx.uid, doors[itemEx.itemid])
                    return true
                end
            end
            doPlayerSendCancel(cid, "The key does not match.")
            return true
        end
        return false
    elseif isInArray(horizontalOpenDoors, item.itemid) then
        local newPosition = toPosition
        newPosition.y = newPosition.y + 1
        local doorPosition = fromPosition
        doorPosition.stackpos = STACKPOS_TOP_MOVEABLE_ITEM_OR_CREATURE
        local doorCreature = getThingfromPos(doorPosition)
        if doorCreature.itemid ~= 0 then
            if getTilePzInfo(doorPosition) and not getTilePzInfo(newPosition) and doorCreature.uid ~= cid then
                doPlayerSendCancel(cid, "Sorry, not possible.")
            else
                doTeleportThing(doorCreature.uid, newPosition, true)
            end
        else
            doTransformItem(item.uid, item.itemid - 1)
        end
        return true
    elseif isInArray(verticalOpenDoors, item.itemid) then
        local newPosition = toPosition
        newPosition.x = newPosition.x + 1
        local doorPosition = fromPosition
        doorPosition.stackpos = STACKPOS_TOP_MOVEABLE_ITEM_OR_CREATURE
        local doorCreature = getThingfromPos(doorPosition)
        if doorCreature.itemid ~= 0 then
            if getTilePzInfo(doorPosition) and not getTilePzInfo(newPosition) and doorCreature.uid ~= cid then
                doPlayerSendCancel(cid, "Sorry, not possible.")
            else
                doTeleportThing(doorCreature.uid, newPosition, true)
            end
        else
            doTransformItem(item.uid, item.itemid - 1)
        end
        return true
    elseif doors[item.itemid] ~= nil then
        if item.actionid == 0 then
            doTransformItem(item.uid, doors[item.itemid])
        else
            doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "It is locked.")
        end
        return true
    end
    return false
end

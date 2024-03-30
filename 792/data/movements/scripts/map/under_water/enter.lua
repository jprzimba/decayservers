function onStepIn(cid, item, position, fromPosition)
    if ((position.x == 345 and position.y == 429 and position.z == 7) or (position.x == 345 and position.y == 430 and position.z == 7)) and item.actionid == 4550 then
        if getPlayerInventoryItemId(cid, 1) == 5461 then
            doTeleportThing(cid, {x = 345, y = 429, z = 8}, false)
        else
            doTeleportThing(cid, {x = position.x - 1, y = position.y, z = position.z}, false)
            doPlayerSendCancel(cid, "You must use a Helmet of The Deep to go underwater.")
        end
    elseif ((position.x == 265 and position.y == 395 and position.z == 7) or (position.x == 266 and position.y == 395 and position.z == 7)) and item.actionid == 4550 then
        if getPlayerInventoryItemId(cid, 1) == 5461 then
            doTeleportThing(cid, {x = 265, y = 395, z = 8}, false)
        else
            doTeleportThing(cid, {x = position.x, y = position.y + 1, z = position.z}, false)
            doPlayerSendCancel(cid, "You must use a Helmet of The Deep to go underwater.")
        end
    elseif ((position.x == 628 and position.y == 675 and position.z == 7) or (position.x == 628 and position.y == 676 and position.z == 7)) and item.actionid == 4550 then
        if getPlayerInventoryItemId(cid, 1) == 5461 then
            doTeleportThing(cid, {x = 627, y = 676, z = 8}, false)
        else
            doTeleportThing(cid, {x = position.x - 1, y = position.y, z = position.z}, false)
            doPlayerSendCancel(cid, "You must use a Helmet of The Deep to go underwater.")
        end
    elseif ((position.x == 537 and position.y == 559 and position.z == 7) or (position.x == 538 and position.y == 559 and position.z == 7)) and item.actionid == 4550 then
        if getPlayerInventoryItemId(cid, 1) == 5461 then
            doTeleportThing(cid, {x = 538, y = 559, z = 8}, false)
        else
            doTeleportThing(cid, {x = position.x, y = position.y - 1, z = position.z}, false)
            doPlayerSendCancel(cid, "You must use a Helmet of The Deep to go underwater.")
        end
    end

    return true
end
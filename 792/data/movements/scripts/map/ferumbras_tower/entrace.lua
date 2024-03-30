function onStepIn(cid, item, position, fromPosition)
    local correctPosition = {x = 648, y = 587, z = 7}
    local teleportPosition = {x = 657, y = 571, z = 9}

    if isPlayer(cid) and position.x == correctPosition.x and position.y == correctPosition.y and position.z == correctPosition.z then
        local tileItem = getTileItemById(position, 473)
        if tileItem then
            doTeleportThing(cid, teleportPosition, false)
        end
    end
    return true
end

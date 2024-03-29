function onUse(cid, item, fromPosition, itemEx, toPosition)
    local wall1 = {x = 1496, y = 1488, z = 5, stackpos = STACKPOS_FIRST_ITEM_ABOVE_GROUNDTILE}
    local wall2 = {x = 1497, y = 1488, z = 5, stackpos = STACKPOS_FIRST_ITEM_ABOVE_GROUNDTILE}
    local getwall1 = getThingfromPos(wall1)
    local getwall2 = getThingfromPos(wall2)

    if item.uid == 7002 then
        if item.itemid == 1945 then
            doRemoveItem(getwall1.uid, 1)
            doRemoveItem(getwall2.uid, 1)
            doTransformItem(item.uid, item.itemid + 1)
        elseif item.itemid == 1946 then
            doCreateItem(1052, 1, wall1)
            doCreateItem(1052, 1, wall2)
            doTransformItem(item.uid, item.itemid - 1)
        else
            doPlayerSendCancel(cid, RETURNVALUE_NOTPOSSIBLE)
        end
    end

    return true
end

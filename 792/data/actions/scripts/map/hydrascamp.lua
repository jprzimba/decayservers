function onUse(cid, item, fromPosition, itemEx, toPosition)
    local laderPosition = {x = 624, y = 433, z = 8, stackpos = STACKPOS_FIRST_ITEM_ABOVE_GROUNDTILE}
    local lader = getThingfromPos(laderPosition)

    if item.uid == 14030 then
        if item.itemid == 1945 and lader.itemid == 0 then
            doCreateItem(1386, 1, laderPosition)
            doTransformItem(item.uid, item.itemid + 1)
        elseif item.itemid == 1946 and lader.itemid == 1386 then
            doRemoveItem(lader.uid, 1)
            doTransformItem(item.uid, item.itemid - 1)
        else
            doPlayerSendCancel(cid, RETURNVALUE_NOTPOSSIBLE)
        end
    end

    return true
end

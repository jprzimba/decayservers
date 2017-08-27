local JUNGLE_GRASS = {2782, 3985}

function onUse(cid, item, fromPosition, itemEx, toPosition)
	if(isInArray(JUNGLE_GRASS, itemEx.itemid)) then
		doTransformItem(itemEx.uid, itemEx.itemid - 1)
		doDecayItem(itemEx.uid)
		return true
	end

	if(itemEx.itemid == 1499) then
		doSendMagicEffect(toPosition, CONST_ME_POFF)
		doRemoveItem(itemEx.uid)
		return true
	end

	return destroyItem(cid, itemEx, toPosition)
end

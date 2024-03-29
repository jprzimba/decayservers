local t = {
	[9000] = {9000, "a demon armor", 2494, 1},
	[9001] = {9000, "a magic sword", 2400, 1},
	[9002] = {9000, "a stonecutter axe", 2431, 1},
	[9003] = {9000, "a thunder hammer", 2421, 1}
}

function onUse(cid, item, fromPosition, itemEx, toPosition)
	local v = t[item.uid]
	if getPlayerStorageValue(cid,v[1]) == -1 and getPlayerFreeCap(cid) >= (getItemWeight(v[3], 1, false)) then
		setPlayerStorageValue(cid, v[1], 1)
		doPlayerAddItem(cid, v[3], v[4])
		doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You have found " .. v[2] .. ".")
	elseif getPlayerStorageValue(cid, v[1]) == 1 then
		doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "It is empty.")
	elseif getPlayerFreeCap(cid) < (getItemWeight(v[3], 1, false)) then
		doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You need " .. getItemWeight(v[3], 1, false) .. ".00 oz in order to get the item")
	end

	return true
end
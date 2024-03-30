local t = {
	[5010] = {5010, "golden armor", 2466, 1},
	[5011] = {5011, "demon shield", 2520, 1},
	[5012] = {5012, "guargian halberd", 2427, 1},
	[5013] = {5013, "platinum amulet", 2171, 1}
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
		doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You need " .. getItemWeight(v[3], 1, false) .. ".00 oz in order to get the item.")
	end

	return true
end

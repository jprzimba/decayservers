function getCreatureLookPosition(cid)
	return getPosByDir(getThingPos(cid), getCreatureLookDirection(cid))
end

function getPositionByDirection(position, direction, size)
	local n = size or 1
	if(direction == NORTH) then
		position.y = position.y - n
	elseif(direction == SOUTH) then
		position.y = position.y + n
	elseif(direction == WEST) then
		position.x = position.x - n
	elseif(direction == EAST) then
		position.x = position.x + n
	elseif(direction == NORTHWEST) then
		position.y = position.y - n
		position.x = position.x - n
	elseif(direction == NORTHEAST) then
		position.y = position.y - n
		position.x = position.x + n
	elseif(direction == SOUTHWEST) then
		position.y = position.y + n
		position.x = position.x - n
	elseif(direction == SOUTHEAST) then
		position.y = position.y + n
		position.x = position.x + n
	end

	return position
end

function getBooleanFromString(input)
	local tmp = type(input)
	if(tmp == 'boolean') then
		return input
	end

	if(tmp == 'number') then
		return input > 0
	end

	local str = string.lower(tostring(input))
	return (str == "yes" or str == "true" or (tonumber(str) ~= nil and tonumber(str) > 0))
end

function doPlayerGiveItem(cid, itemid, count, charges)
	local hasCharges = (isItemRune(itemid) == TRUE or isItemFluidContainer(itemid) == TRUE)
	if(hasCharges and charges == nil) then
		charges = 1
	end
	while count > 0 do
		local tempcount = 1
		if(hasCharges) then
			tempcount = charges
		end
		if(isItemStackable(itemid) == TRUE) then
			tempcount = math.min (100, count)
   		end
	   	local ret = doPlayerAddItem(cid, itemid, tempcount)
	   	if(ret == LUA_ERROR) then
			ret = doCreateItem(itemid, tempcount, getPlayerPosition(cid))
		end
		if(ret ~= LUA_ERROR) then
			if(hasCharges) then
				count = count - 1
			else
				count = count - tempcount
			end
		else
			return LUA_ERROR
		end
	end
	return LUA_NO_ERROR
end

function doPlayerTakeItem(cid, itemid, count)
	if(getPlayerItemCount(cid,itemid) >= count) then
		while count > 0 do
			local tempcount = 0
			if(isItemStackable(itemid) == TRUE) then
				tempcount = math.min (100, count)
			else
				tempcount = 1
			end
			local ret = doPlayerRemoveItem(cid, itemid, tempcount)
			if(ret ~= LUA_ERROR) then
				count = count-tempcount
			else
				return LUA_ERROR
			end
		end
		if(count == 0) then
			return LUA_NO_ERROR
		end
	else
		return LUA_ERROR
	end
end

function doPlayerBuyItem(cid, itemid, count, cost, charges)
	if(doPlayerRemoveMoney(cid, cost) == TRUE) then
		return doPlayerGiveItem(cid, itemid, count, charges)
	end
	return LUA_ERROR
end

function doPlayerSellItem(cid, itemid, count, cost)
	if(doPlayerTakeItem(cid, itemid, count) == LUA_NO_ERROR) then
		if doPlayerAddMoney(cid, cost) ~= TRUE then
			error('Could not add money to ' .. getPlayerName(cid) .. '(' .. cost .. 'gp)')
		end
		return LUA_NO_ERROR
	end
	return LUA_ERROR
end

function isInRange(pos, fromPos, toPos)
	if pos.x >= fromPos.x and pos.y >= fromPos.y and pos.z >= fromPos.z and pos.x <= toPos.x and pos.y <= toPos.y and pos.z <= toPos.z then
		return TRUE
	end
	return FALSE
end

function isPremium(cid)
	return (isPlayer(cid) == TRUE and (getPlayerPremiumDays(cid) > 0 or getConfigInfo('freePremium') == "yes")) and TRUE or FALSE
end

function getMonthDayEnding(day)
	if day == "01" or day == "21" or day == "31" then
		return "st"
	elseif day == "02" or day == "22" then
		return "nd"
	elseif day == "03" or day == "23" then
		return "rd"
	else
		return "th"
	end
end

function getMonthString(m)
	return os.date("%B", os.time{year = 1970, month = m, day = 1})
end

function getArticle(str)
	return str:find("[AaEeIiOoUuYy]") == 1 and "an" or "a"
end

function isNumber(str)
	return tonumber(str) ~= nil and TRUE or FALSE
end

function getDistanceBetween(firstPosition, secondPosition)
	local xDif = math.abs(firstPosition.x - secondPosition.x)
	local yDif = math.abs(firstPosition.y - secondPosition.y)

	local posDif = math.max(xDif, yDif)
	if(firstPosition.z ~= secondPosition.z) then
		posDif = posDif + 9 + 6
	end
	return posDif
end

function doPlayerAddAddons(cid, addon)
	for i = 0, table.maxn(maleOutfits) do
		doPlayerAddOutfit(cid, maleOutfits[i], addon)
	end

	for i = 0, table.maxn(femaleOutfits) do
		doPlayerAddOutfit(cid, femaleOutfits[i], addon)
	end
end

function isSorcerer(cid)
	if(isPlayer(cid) == FALSE) then
		debugPrint("isSorcerer: Player not found.")
		return false
	end

	return (isInArray({1,5}, getPlayerVocation(cid)) == TRUE)
end

function isDruid(cid)
	if(isPlayer(cid) == FALSE) then
		debugPrint("isDruid: Player not found.")
		return false
	end

	return (isInArray({2,6}, getPlayerVocation(cid)) == TRUE)
end

function isPaladin(cid)
	if(isPlayer(cid) == FALSE) then
		debugPrint("isPaladin: Player not found.")
		return false
	end

	return (isInArray({3,7}, getPlayerVocation(cid)) == TRUE)
end

function isKnight(cid)
	if(isPlayer(cid) == FALSE) then
		debugPrint("isKnight: Player not found.")
		return false
	end

	return (isInArray({4,8}, getPlayerVocation(cid)) == TRUE)
end

function getConfigInfo(info)
	if (type(info) ~= 'string') then return nil end

	dofile('config.lua')
	return _G[info]
end

function table.find(table, value)
	for i,v in pairs(table) do
		if (v == value) then
			return i
		end
	end
	return nil
end

function getPlayerMoney(cid)
	return ((getPlayerItemCount(cid, ITEM_CRYSTAL_COIN) * 10000) + (getPlayerItemCount(cid, ITEM_PLATINUM_COIN) * 100) + getPlayerItemCount(cid, ITEM_GOLD_COIN))
end

function doPlayerWithdrawMoney(cid, amount)
	local balance = getPlayerBalance(cid)
	if(amount > balance or not doPlayerAddMoney(cid, amount)) then
		return false
	end

	doPlayerSetBalance(cid, balance - amount)
	return true
end

function doPlayerDepositMoney(cid, amount)
	if(not doPlayerRemoveMoney(cid, amount)) then
		return false
	end

	doPlayerSetBalance(cid, getPlayerBalance(cid) + amount)
	return true
end
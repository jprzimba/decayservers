function isPlayerPremiumCallback(cid)
	if getPlayerPremiumDays(cid) > 0 then
		return true
	end
	return false
end

function msgcontains(message, keyword)
	local a, b = string.find(message, keyword)
	if a == nil or b == nil then
		return false
	end
	return true
end

function getDistanceToCreature(cid)
	if isPlayer(cid) == FALSE or cid == 0 or cid == nil then
		return FALSE
	end
	local creaturePos = getCreaturePosition(cid)
	local cx = creaturePos.x
	local cy = creaturePos.y
	local cz = creaturePos.z
	local sx, sy, sz = selfGetPosition()
	if cx == nil or cz ~= sz then
		return 100
	end
	return math.max(math.abs(sx - cx), math.abs(sy - cy))
end

function doPosRemoveItem(_itemid, n, position)
	local thing = getThingfromPos({x = position.x, y = position.y, z = position.z, stackpos = 1})
	if thing.itemid == _itemid then
		doRemoveItem(thing.uid, n)
	else
		return false
	end
	return true
end

-- getCount function by Jiddo
function getCount(msg)
	b, e = string.find(msg, "%d+")
	
	if b == nil or e == nil then
		count = 1
	else
		count = tonumber(string.sub(msg, b, e))
	end
	
	return count
end

function buy(cid, itemid, count, cost)
	cost = count*cost
	amount = count
	if doPlayerRemoveMoney(cid, cost) then
		if getItemStackable(itemid) then
			while count > 100 do
				doPlayerAddItem(cid, itemid, 100)
				count = count - 100
			end
			
			doPlayerAddItem(cid, itemid, count) -- add the last items, if there is left
		else
			while count > 0 do
				doPlayerAddItem(cid, itemid, 1)
				count = count - 1
			end
		end

		if itemid ~= 2595 then 	
			if count <= 1 then
				selfSay('Here is your '.. getItemName(itemid) .. '!')
			else
				selfSay('Here are your '.. amount ..' '.. getItemName(itemid) .. 's!')		
			end
		 else
			selfSay('Here you are. Don\'t forget to write the name and the address of the receiver on the label. The label has to be in the parcelbefore you put the parcel in a mailbox.')
		end
	else
		selfSay('Sorry, you do not have enough money.')
	end
end

function buyFluidContainer(cid, itemid, count, cost, fluidtype)
	cost = count*cost
	amount = count
	if doPlayerRemoveMoney(cid, cost) then
		while count > 0 do
			doPlayerAddItem(cid, itemid, fluidtype)
			count = count - 1
		end
		
		if amount <= 1 then
			selfSay('Here is your '.. getItemName(itemid) .. '!')
		else
			selfSay('Here are your '.. amount ..' '.. getItemName(itemid) .. 's!')		
		end
	else
		selfSay('Sorry, you do not have enough money.')
	end
end

function buyContainer(cid, container, itemid, count, money)
	if doPlayerRemoveMoney(cid, money) then
		bp = doPlayerAddItem(cid, container, 1)
		x = 0
		
		while x < 20 do
			doAddContainerItem(bp, itemid, count)
			x = x + 1
		end
		
		selfSay('Here you are.')
	else
		selfSay('Sorry, you don\'t have enough money.')
	end
end 

function sell(cid, itemid, count, cost)
	if doPlayerRemoveItem(cid, itemid, count) then
		cost2 = cost*count
		selfSay('Thanks for this '.. getItemName(itemid) .. '!')
		if not doPlayerAddMoney(cid, cost2) then
			error('Could not add money to ' .. getPlayerName(cid) .. '(' .. cost .. 'gp)')
		end
		return true
	else
		selfSay('Sorry, You not have this item')
	end
	return false
end

function pay(cid, cost)
	if doPlayerRemoveMoney(cid, cost) then
		return true
	else
		return false
	end
end

-- Including the Advanced NPC System
dofile('data/npc/lib/npcsystem/npcsystem.lua')
function onUse(cid, item, fromPosition, itemEx, toPosition)
	if item.uid == 7001 and item.itemid == 1945 then
		firstPlayerPosition = {x = 1040, y = 987, z = 8, stackpos = STACKPOS_TOP_CREATURE}
		firstPlayer = getThingfromPosition(firstPlayerPosition)

		secondPlayerPosition = {x = 1040, y = 989, z = 8, stackpos = STACKPOS_TOP_CREATURE}
		secondPlayer = getThingfromPosition(secondPlayerPosition)

		if firstPlayer.itemid > 0 and secondPlayer.itemid > 0 then
			minArenaLevel = 25
			firstPlayerlevel = getPlayerLevel(firstPlayer.uid)
			secondPlayerlevel = getPlayerLevel(secondPlayer.uid)

			if firstPlayerlevel >= minArenaLevel and secondPlayerlevel >= minArenaLevel then
				for arenax = 1041, 1052 do
					for arenay = 985, 991 do
						arenaPosition = {x = arenax, y = arenay, z = 9, stackpos = STACKPOS_TOP_CREATURE}
						arenaCreature = getThingfromPosition(arenaPosition)

						if arenaCreature.itemid > 0 then
							doPlayerSendCancel(cid, "Wait for current duel to end.")
							return true
						end
					end
				end

				newFirstPlayerPosition = {x = 1043, y = 988, z = 9}
				newSecondPlayerPosition = {x = 1050, y = 988, z = 9}

				doSendMagicEffect(firstPlayerPosition, CONST_ME_POFF)
				doSendMagicEffect(secondPlayerPosition, CONST_ME_POFF)

				doTeleportThing(firstPlayer.uid, newFirstPlayerPosition)
				doTeleportThing(secondPlayer.uid, newSecondPlayerPosition)

				doSendMagicEffect(newFirstPlayerPosition, CONST_ME_ENERGYAREA)
				doSendMagicEffect(newSecondPlayerPosition, CONST_ME_ENERGYAREA)

				doPlayerSendTextMessage(firstPlayer.uid, MESSAGE_STATUS_WARNING, "FIGHT!")
				doPlayerSendTextMessage(secondPlayer.uid, MESSAGE_STATUS_WARNING, "FIGHT!")
			else
				doPlayerSendCancel(cid, "Both fighters must have level 25.")
			end
		else
			doPlayerSendCancel(cid, "You need 2 players for a duel.")
		end
	else
		return false
   	end

	return true
end

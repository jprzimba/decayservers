function onSay(cid, words, param)
	if isPlayer(cid) == TRUE and param ~= "" and getPlayerAccess(cid) > 0 then
		if param == "custom" then
			doPlayerSetSex(cid, 2)
		elseif param == "male" then
			doPlayerSetSex(cid, 1)
		elseif param == "female" then
			doPlayerSetSex(cid, 0)
		end
	else
		doPlayerSetSex(cid, param)
		if getPlayerPremiumDays(cid) > 2 then
			doPlayerRemovePremiumDays(cid, 3)
			if getPlayerSex(cid) == 0 then
				doPlayerSetSex(cid, 1)
			else
				doPlayerSetSex(cid, 0)
			end
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You have changed your sex and lost three days of premium account.")
		else
			doPlayerSendCancel(cid, "You do not have enough premium days, changing sex costs three of your premium days.")
			doSendMagicEffect(getPlayerPosition(cid), CONST_ME_POFF)
		end
	end
	return false
end
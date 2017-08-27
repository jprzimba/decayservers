function executeClean()
	doCleanMap()
	broadcastMessage("Game map cleaned, next clean in 2 hours.", MESSAGE_STATUS_WARNING)
	return true
end

function onTime()
	broadcastMessage("Game map cleaning within 30 seconds, please pick up your items!", MESSAGE_STATUS_WARNING)
	addEvent(executeClean, 30000)
	return true
end

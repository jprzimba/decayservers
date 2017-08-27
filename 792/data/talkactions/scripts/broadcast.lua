function onSay(cid, words, param)
	if(param == '') then
		return false
	end

	broadcastMessage(param, MESSAGE_STATUS_WARNING)
	return false
end

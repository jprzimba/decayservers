local endPositions = {
   {x = 306, y = 457, z = 8, stackpos = STACKPOS_TOP_MOVEABLE_ITEM_OR_CREATURE},
   {x = 307, y = 457, z = 8, stackpos = STACKPOS_TOP_MOVEABLE_ITEM_OR_CREATURE},
   {x = 309, y = 457, z = 8, stackpos = STACKPOS_TOP_MOVEABLE_ITEM_OR_CREATURE},
   {x = 310, y = 457, z = 8, stackpos = STACKPOS_TOP_MOVEABLE_ITEM_OR_CREATURE}
}

local playerPositions = {
   {x = 395, y = 533, z = 8, stackpos = STACKPOS_TOP_CREATURE},
   {x = 398, y = 530, z = 8, stackpos = STACKPOS_TOP_CREATURE},
   {x = 401, y = 533, z = 8, stackpos = STACKPOS_TOP_CREATURE},
   {x = 398, y = 536, z = 8, stackpos = STACKPOS_TOP_CREATURE}
}

local itemPositions = {
   { x = 394, y = 533, z = 8, stackpos = STACKPOS_TOP_MOVEABLE_ITEM_OR_CREATURE},
   { x = 398, y = 529, z = 8, stackpos = STACKPOS_TOP_MOVEABLE_ITEM_OR_CREATURE},
   { x = 402, y = 533, z = 8, stackpos = STACKPOS_TOP_MOVEABLE_ITEM_OR_CREATURE},
   { x = 398, y = 537, z = 8, stackpos = STACKPOS_TOP_MOVEABLE_ITEM_OR_CREATURE}
}

local items = {}
local players = {}

function onUse(cid, item, fromPosition, itemEx, toPosition)
   for i = 1, 4 do
       items[i] = getThingfromPos(itemPositions[i])
       players[i] = getThingfromPos(playerPositions[i])
   end

   if item.uid == 9010 and item.itemid == 1945 then
       local itemIds = {2674, 2455, 2175, 2376}
       local playerVocations = {2, 3, 1, 4}
       local allItemsExist = true
       local allPlayersExist = true
       local correctVocations = true

       for i = 1, 4 do
           if items[i].itemid ~= itemIds[i] then
               allItemsExist = false
               break
           end
           if players[i].itemid <= 0 then
               allPlayersExist = false
               break
           end
           if getPlayerVocation(players[i].uid) ~= playerVocations[i] then
               correctVocations = false
               break
           end
       end

       if allItemsExist and allPlayersExist and correctVocations then
           for i = 1, 4 do
               doRemoveItem(items[i].uid, 1)
               doSendMagicEffect(itemPositions[i], CONST_ME_MAGIC_RED)
               doTeleportThing(players[i].uid, endPositions[i])
               doSendMagicEffect(playerPositions[i], CONST_ME_TELEPORT)
               doSendMagicEffect(endPositions[i], CONST_ME_TELEPORT)
           end
           doTransformItem(item.uid, item.itemid + 1)
       else
         doPlayerSendCancel(cid, "Make sure all players are present and have the correct vocations.")
       end
   elseif item.uid == 9010 and item.itemid == 1946 then
       doTransformItem(item.uid, item.itemid - 1)
   end

   return true
end

function onUse(cid, item, fromPosition, itemEx, toPosition)
   if item.uid == 7000 then
       if item.itemid == 1946 then
           player1pos = { x = 500, y = 755, z = 9, stackpos = STACKPOS_TOP_CREATURE}
           player1 = getThingfromPos(player1pos)

           player2pos = {x = 499, y = 755, z = 9, stackpos = STACKPOS_TOP_CREATURE}
           player2 = getThingfromPos(player2pos)

           player3pos = {x = 498, y = 755, z = 9, stackpos=STACKPOS_TOP_CREATURE}
           player3 = getThingfromPos(player3pos)

           player4pos = {x = 497, y = 755, z = 9, stackpos=STACKPOS_TOP_CREATURE}
           player4 = getThingfromPos(player4pos)

           if player1.itemid > 0 and player2.itemid > 0 and player3.itemid > 0 and player4.itemid > 0 then
               player1level = getPlayerLevel(player1.uid)
               player2level = getPlayerLevel(player2.uid)
               player3level = getPlayerLevel(player3.uid)
               player4level = getPlayerLevel(player4.uid)
               questlevel = 100

               if player1level >= questlevel and player2level >= questlevel and player3level >= questlevel and player4level >= questlevel then
                   queststatus1 = getPlayerStorageValue(player1.uid, 100)
                   queststatus2 = getPlayerStorageValue(player2.uid, 100)
                   queststatus3 = getPlayerStorageValue(player3.uid, 100)
                   queststatus4 = getPlayerStorageValue(player4.uid, 100)

                   if queststatus1 == -1 and queststatus2 == -1 and queststatus3 == -1 and queststatus4 == -1 then
                       demon1pos = {x=498, y=753, z=10}
                       demon2pos = {x=500, y=753, z=10}
                       demon3pos = {x=499, y=757, z=10}
                       demon4pos = {x=497, y=757, z=10}
                       demon5pos = {x=502, y=755, z=10}
                       demon6pos = {x=501, y=755, z=10}

                       doSummonCreature("Demon", demon1pos)
                       doSummonCreature("Demon", demon2pos)
                       doSummonCreature("Demon", demon3pos)
                       doSummonCreature("Demon", demon4pos)
                       doSummonCreature("Demon", demon5pos)
                       doSummonCreature("Demon", demon6pos)

                       nplayer1pos = {x=500, y=755, z=10}
                       nplayer2pos = {x=499, y=755, z=10}
                       nplayer3pos = {x=498, y=755, z=10}
                       nplayer4pos = {x=497, y=755, z=10}

                       doSendMagicEffect(player1pos, 2)
                       doSendMagicEffect(player2pos, 2)
                       doSendMagicEffect(player3pos, 2)
                       doSendMagicEffect(player4pos, 2)

                       doTeleportThing(player1.uid, nplayer1pos, CONST_ME_POFF)
                       doTeleportThing(player2.uid, nplayer2pos, CONST_ME_POFF)
                       doTeleportThing(player3.uid, nplayer3pos, CONST_ME_POFF)
                       doTeleportThing(player4.uid, nplayer4pos, CONST_ME_POFF)

                       doSendMagicEffect(nplayer1pos, CONST_ME_TELEPORT)
                       doSendMagicEffect(nplayer2pos, CONST_ME_TELEPORT)
                       doSendMagicEffect(nplayer3pos, CONST_ME_TELEPORT)
                       doSendMagicEffect(nplayer4pos, CONST_ME_TELEPORT)

                       doTransformItem(item.uid, 1945)
                   else
                       doPlayerSendCancel(cid, "The lever has already been activated.")
                   end
               else
                   doPlayerSendCancel(cid, "The nearby players are not at the required level to activate the lever.")
               end
           else
               doPlayerSendCancel(cid, "There are not enough players near the lever.")
           end
       else
           doPlayerSendCancel(cid, "The lever is already in the correct position.")
       end
   end
   return true
end

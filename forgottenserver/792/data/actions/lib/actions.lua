FOODS = {	
	[2328] = {84, "Gulp."},	[2362] = {48, "Yum."}, [2666] = {180, "Munch."}, [2667] = {144, "Munch."},
	[2668] = {120, "Mmmm."}, [2669] = {204, "Munch."}, [2670] = {48, "Gulp."}, [2671] = {360, "Chomp."},
	[2672] = {720, "Chomp."}, [2673] = {60, "Yum."}, [2674] = {72, "Yum."}, [2675] = {156, "Yum."}, 
	[2676] = {96, "Yum."}, [2677] = {12, "Yum."}, [2678] = {216, "Slurp."}, [2679] = {12, "Yum."}, 
	[2680] = {24, "Yum."}, [2681] = {108, "Yum."}, [2682] = {240, "Yum."}, [2683] = {204, "Munch."}, 
	[2684] = {60, "Crunch."}, [2685] = {72, "Munch."}, [2686] = {108, "Crunch."}, [2687] = {24, "Crunch."},
	[2688] = {24, "Mmmm."}, [2689] = {120, "Crunch."}, [2690] = {72, "Crunch."}, [2691] = {96, "Crunch."}, 
	[2695] = {72, "Gulp."}, [2696] = {108, "Smack."}, [2769] = {60, "Crunch."}, [2787] = {108, "Crunch."},
	[2788] = {48, "Crunch."}, [2789] = {264, "Crunch."}, [2790] = {360, "Crunch."}, [2791] = {108, "Crunch."},
	[2792] = {72, "Crunch."}, [2793] = {144, "Crunch."}, [2794] = {36, "Crunch."}, [2795] = {432, "Crunch."},
	[2796] = {300, "Crunch."}
}

function destroyItem(cid, itemEx, toPosition)
	if itemEx.uid <= 65535 or itemEx.actionid > 0 then
		return FALSE
	end

	if (itemEx.itemid >= 1724 and itemEx.itemid <= 1741) or (itemEx.itemid >= 2581 and itemEx.itemid <= 2588) or itemEx.itemid == 1770 or itemEx.itemid == 2098 or itemEx.itemid == 1774 or itemEx.itemid == 1775 or itemEx.itemid == 2064 or (itemEx.itemid >= 1747 and itemEx.itemid <= 1753) or (itemEx.itemid >= 1714 and itemEx.itemid <= 1717) or (itemEx.itemid >= 1650 and itemEx.itemid <= 1653) or (itemEx.itemid >= 1666 and itemEx.itemid <= 1677) or (itemEx.itemid >= 1614 and itemEx.itemid <= 1616) or (itemEx.itemid >= 3813 and itemEx.itemid <= 3820) or (itemEx.itemid >= 3807 and itemEx.itemid <= 3810) or (itemEx.itemid >= 2080 and itemEx.itemid <= 2085) or (itemEx.itemid >= 2116 and itemEx.itemid <= 2119) or itemEx.itemid == 2094 or itemEx.itemid == 2095 or itemEx.itemid == 1619 or itemEx.itemid == 2602 or itemEx.itemid == 3805 or itemEx.itemid == 3806 then
		if math.random(1, 7) == 1 then
			if itemEx.itemid == 1738 or itemEx.itemid == 1739 or (itemEx.itemid >= 2581 and itemEx.itemid <= 2588) or itemEx.itemid == 1770 or itemEx.itemid == 2098 or itemEx.itemid == 1774 or itemEx.itemid == 1775 or itemEx.itemid == 2064 then
				doCreateItem(2250, 1, toPosition)
			elseif (itemEx.itemid >= 1747 and itemEx.itemid <= 1749) or itemEx.itemid == 1740 then
				doCreateItem(2251, 1, toPosition)
			elseif (itemEx.itemid >= 1714 and itemEx.itemid <= 1717) then
				doCreateItem(2252, 1, toPosition)
			elseif (itemEx.itemid >= 1650 and itemEx.itemid <= 1653) or (itemEx.itemid >= 1666 and itemEx.itemid <= 1677) or (itemEx.itemid >= 1614 and itemEx.itemid <= 1616) or (itemEx.itemid >= 3813 and itemEx.itemid <= 3820) or (itemEx.itemid >= 3807 and itemEx.itemid <= 3810) then
				doCreateItem(2253, 1, toPosition)
			elseif (itemEx.itemid >= 1724 and itemEx.itemid <= 1737) or (itemEx.itemid >= 2080 and itemEx.itemid <= 2085) or (itemEx.itemid >= 2116 and itemEx.itemid <= 2119) or itemEx.itemid == 2094 or itemEx.itemid == 2095 then
				doCreateItem(2254, 1, toPosition)
			elseif (itemEx.itemid >= 1750 and itemEx.itemid <= 1753) or itemEx.itemid == 1619 or itemEx.itemid == 1741 then
				doCreateItem(2255, 1, toPosition)
			elseif itemEx.itemid == 2602 then
				doCreateItem(2257, 1, toPosition)
			elseif itemEx.itemid == 3805 or itemEx.itemid == 3806 then
				doCreateItem(2259, 1, toPosition)
			end
			doRemoveItem(itemEx.uid, 1)
		end
		doSendMagicEffect(toPosition, CONST_ME_POFF)
		return TRUE
	end
	return FALSE
end
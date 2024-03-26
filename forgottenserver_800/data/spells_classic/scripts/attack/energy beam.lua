local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_ENERGYDAMAGE)
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_ENERGYHIT)
setCombatFormula(combat, COMBAT_FORMULA_LEVELMAGIC, -.8, 0, 0, 0)

local area = createCombatArea(AREA_BEAM5, AREADIAGONAL_BEAM5)
setCombatArea(combat, area)

function onCastSpell(cid, var)
	return doCombat(cid, combat, var)
end

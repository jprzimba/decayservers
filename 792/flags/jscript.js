function biAND(flags, flag)
{
	for(var i = 0; i < flags.digits.length; ++i)
	{
		if(i < flag.digits.length)
		{
			var r1 = flags.digits[i]
			var r2 = flag.digits[i]
			var result = r1 & r2
			if(result != 0)
				return true
		}
	}
	
	return false
}

function calcFlags()
{
	var flags = biFromDecimal("0")
	for(var i = 0; i < document.flags.elements.length; i++)
	{
		if(document.flags.elements[i].type == "checkbox" && document.flags.elements[i].checked)
		{
			var flag = biFromDecimal(document.flags.elements[i].value)
			flags = biAdd(flags, flag)
		}
	}

	document.flags.result.value = biToString(flags, 10)
}

function calcCheckboxes()
{
	var flags = biFromDecimal(document.flags.result.value)
	for(var i = 0; i < document.flags.elements.length; i++)
	{
		var flag = biFromDecimal(document.flags.elements[i].value)		
		document.flags.elements[i].checked = biAND(flags, flag)
	}
}

<?php
/*
    Copyright (C) 2007  Nicaw

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
include ("include.inc.php");
$ptitle= "Spells - $cfg[server_name]";
include ("header.inc.php");
?>
<div id="content">
<div class="top">Spells</div>
<div class="mid">
<form method="get" action="spells.php"> 
<input type="text" name="spell"/> 
<input type="submit" value="Search"/> 
</form>
<div style="height: 500px; overflow: auto; margin: 10px;">
<?php
$SpellsXML = @simplexml_load_file($cfg['dirdata'].'spells/spells.xml');
if ($SpellsXML === false) throw new aacException($cfg['dirdata'].'spells/spells.xml not found');
foreach ($SpellsXML->instant as $spell){
	if (!empty($_GET['spell']) and (stristr($spell['name'], $_GET['spell']) or stristr($spell['words'], $_GET['spell'])) or !isset($_GET['spell'])){
		echo '<table><tr><td width="150px"><b>Spell Name</b></td><td>'.$spell['name'].'</td></tr>';
		echo '<tr><td><b>Type</b></td><td>Instant</td></tr>';
		echo '<tr><td><b>Words</b></td><td>'.$spell['words'].'</td></tr>';
		if ($spell['lvl'] > 0)
			echo '<tr><td><b>Required Level</b></td><td>'.$spell['lvl'].'</td></tr>';
		if ($spell['maglv'] > 0)
			echo '<tr><td><b>Magic Level</b></td><td>'.$spell['maglv'].'</td></tr>';
		echo '<tr><td><b>Mana</b></td><td>'.$spell['mana'].'</td></tr>';
		echo '<tr><td><b>Premium</b></td><td>'.(($spell['prem'] == 1)?'yes':'no').'</td></tr>';
		echo '</table><hr/>';
	}
}
foreach ($SpellsXML->conjure as $spell){
	if (!empty($_GET['spell']) and (stristr($spell['name'], $_GET['spell']) or stristr($spell['words'], $_GET['spell'])) or !isset($_GET['spell'])){
		echo '<table><tr><td width="150px"><b>Spell Name</b></td><td>'.$spell['name'].'</td></tr>';
		echo '<tr><td><b>Type</b></td><td>Conjure</td></tr>';
		echo '<tr><td><b>Words</b></td><td>'.$spell['words'].'</td></tr>';
		if ($spell['lvl'] > 0)
			echo '<tr><td><b>Required Level</b></td><td>'.$spell['lvl'].'</td></tr>';
		if ($spell['maglv'] > 0)
			echo '<tr><td><b>Magic Level</b></td><td>'.$spell['maglv'].'</td></tr>';
		echo '<tr><td><b>Mana</b></td><td>'.$spell['mana'].'</td></tr>';
		echo '<tr><td><b>Premium</b></td><td>'.(($spell['prem'] == 1)?'yes':'no').'</td></tr>';
		echo '<tr><td><b>Soul Points</b></td><td>'.$spell['soul'].'</td></tr>';
		echo '<tr><td><b>Conjure Count</b></td><td>'.$spell['conjureCount'].'</td></tr>';
		echo '</table><hr/>';
	}
}
?>
</div>
</div>
<div class="bot"></div>
</div>
<?php include ("footer.inc.php");?>
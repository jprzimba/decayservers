<?php 
/*
    Copyright (C) 2007 - 2009  Nicaw

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
$ptitle= "Quests - $cfg[server_name]";
include ("header.inc.php");
?>
<div id="content">
<div class="top">Quests</div>
<div class="mid">
<?php 
$QuestXML = @simplexml_load_file('quests.xml');
if ($QuestXML === false) throw new aacException('quests.xml not found');
echo '<table><tr class="color0"><td><b>Quest</b></td><td><b>Level</b></td><td><b>Description</b></td><td><b>Rewards</b></td></tr>';
foreach($QuestXML->quest as $quest){
	$i++;
	echo '<tr '.getStyle($i).'><td>'.htmlspecialchars($quest['title']).'</td><td>'.htmlspecialchars($quest['level']).'</td><td>'.htmlspecialchars($quest->description).'</td><td>'.htmlspecialchars($quest->rewards).'</td></tr>';
}
echo '</table>';
?>
</div>
<div class="bot"></div>
</div>
<?php include ("footer.inc.php");?>
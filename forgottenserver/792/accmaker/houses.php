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
$ptitle="Houses - $cfg[server_name]";
include ("header.inc.php");
if (!is_file($cfg['dirdata'].$cfg['house_file']))
	throw new aacException('House file not found: '.$cfg['dirdata'].$cfg['house_file']);
?>
<div id="content">
<div class="top">Houses</div>
<div class="mid">
<div style="height: 500px; overflow: auto; margin: 10px;">
<table>
<tr class="color0"><td width="35%"><b>House</b></td><td width="25%"><b>Location</b></td><td width="25%"><b>Owner</b></td><td><b>Size</b></td><td><b>Rent</b></td></tr>
<?php 
$HousesXML = simplexml_load_file($cfg['dirdata'].$cfg['house_file']);
$MySQL = AAC::$SQL;
$MySQL->myQuery('SELECT `players`.`name`, `houses`.`id` FROM `players`, `houses` WHERE `houses`.`owner` = `players`.`id`;');

while ($row = $MySQL->fetch_array()){
	$houses[(int)$row['id']] = $row['name'];
}
foreach ($HousesXML->house as $house){
	$i++;
	$list.= '<tr '.getStyle($i).'><td>'.htmlspecialchars($house['name']).'</td><td>'.htmlspecialchars($cfg['temple'][(int)$house['townid']]['name']).'</td><td>'.$houses[(int)$house['houseid']].'</td><td>'.$house['size'].'</td><td>'.$house['rent'].'</td></tr>'."\r\n";
}
echo $list;

?>
</table>
</div>
</div>
<div class="bot"></div>
</div>
<?php include('footer.inc.php');?>
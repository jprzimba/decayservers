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
$ptitle="Bans - $cfg[server_name]";
include ("header.inc.php");
// maximum ban time to display, set 0 to show all bans
$cfg['max_ban_time'] = 2*30*24*60*60; //2 months
?>
<div id="content">
<div class="top">Server Bans</div>
<div class="mid">
<?php
$SQL = AAC::$SQL;
$SQL->myQuery('SELECT players.name, bans.expires, bans.active FROM bans, players WHERE bans.type = 2 AND players.id = bans.value OR bans.type = 3 AND players.account_id = bans.value ORDER BY expires ASC');

echo '<table style="width:100%">'."\n";
echo '<tr class="color0"><td style="width:25%"><b>Name</b></td><td style="width:50%"><b>Ban Ends</b></td><td style="width:25%"><b>Time Left</b></td></tr>'."\n";
while ($ban = $SQL->fetch_array()){
	if(!$ban['active']) continue;
	$time = date("jS F Y H:i:s",(int) $ban['expires']);
	$d = floor(($ban['expires'] - time())/(24*3600));
	$h = floor(($ban['expires'] - time() - $d * 24*3600)/3600);
	if ($d != 0)
		$timeleft = $d.'d '.$h.'h';
	else
		$timeleft = $h.'h';
	if ($ban['expires'] - time() > 0 && (($ban['expires'] - time()) < $cfg['max_ban_time'] || $cfg['max_ban_time'] == 0)){
		$i++;
		echo '<tr '.getStyle($i).'><td>'.$ban['name'].'</td><td>'.$time.'</td><td>'.$timeleft.'</td></tr>'."\n";
	}
}
echo '</table>'."\n";
?>
</div>
<div class="bot"></div>
</div>
<?php include ("footer.inc.php");?>
<?php 
/*
    Copyright (C) 2006  Nicaw

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
include('config.inc.php');
include('include.inc.php');
$ptitle="Server Info - $cfg[server_name]";
include ("header.inc.php");
function getinfo($host='localhost',$port=7171){
		// connects to server
        $socket = @fsockopen($host, $port, $errorCode, $errorString, 1);
		$data = '';
        // if connected then checking statistics
        if($socket)
        {
            // sets 1 second timeout for reading and writing
            stream_set_timeout($socket, 1);

            // sends packet with request
            // 06 - length of packet, 255, 255 is the comamnd identifier, 'info' is a request
            fwrite($socket, chr(6).chr(0).chr(255).chr(255).'info');

            // reads respond
			while (!feof($socket)){
				$data .= fread($socket, 128);
			}

			// closing connection to current server
			fclose($socket);
		}
	return $data;
}

if ($cfg['status_update_interval'] < 60) $cfg['status_update_interval'] = 60;
$modtime = filemtime('status.xml');
if (time() - $modtime > $cfg['status_update_interval'] || $modtime > time()){
	$info = getinfo($cfg['server_ip'], $cfg['server_port']);
	if (!empty($info)) file_put_contents('status.xml',$info);
}else $info = file_get_contents('status.xml');
if (!empty($info)) {
$infoXML = simplexml_load_string($info);

	$up = (int)$infoXML->serverinfo['uptime'];
	$online = (int)$infoXML->players['online'];
	$max = (int)$infoXML->players['max'];

	$h = floor($up/3600);
	$up = $up - $h*3600;
	$m = floor($up/60);
	$up = $up - $m*60;
	if ($h < 10) {$h = "0".$h;}
	if ($m < 10) {$m = "0".$m;}
?>
<div id="content">
<div class="top">Server Info</div>
<div class="mid">
    <table>
<tr><td>Server name</td><td><b><?=$infoXML->serverinfo['servername']?></b></td></tr>
<tr><td>IP: </td><td><b><?=$infoXML->serverinfo['ip']?></b></td></tr>
<tr><td>Port: </td><td><b><?=$infoXML->serverinfo['port']?></b></td></tr>
<tr><td>Server version</td><td><b><?=$infoXML->serverinfo['server']?> <?=$infoXML->serverinfo['version']?></b></td></tr>
<tr><td>Client</td><td><b><?=$infoXML->serverinfo['client']?></b></td></tr>

<tr><td>Website URL</td><td><b><?=$infoXML->serverinfo['url']?></b></td></tr>
<tr><td>Location</td><td><b><?=$infoXML->serverinfo['location']?></b></td></tr>
<tr><td>Owner</td><td><b><?=$infoXML->owner['name']?> (<?=$infoXML->owner['email']?>)</b></td></tr>

<tr><td>Map</td><td><b><?=$infoXML->map['name']?>, Size: (<?=$infoXML->map['width']?>x<?=$infoXML->map['height']?>)</b></td></tr>
<tr><td>Map Size</td><td><b><?=$infoXML->map['width']?>x<?=$infoXML->map['height']?></b></td></tr>
<tr><td>Map Author</td><td><b><?=$infoXML->map['author']?></b></td></tr>
<tr><td>Monsters</td><td><b><?=$infoXML->monsters['total']?></b></td></tr>

</table>

<pre>
<?=htmlentities($infoXML->motd)?>
</pre>
<?php 
} else {
	echo "<span class=\"offline\">Server Offline</span>\n";
}
?>
</div>

<div class="bot"></div>
</div>
<?include ('footer.inc.php');?>
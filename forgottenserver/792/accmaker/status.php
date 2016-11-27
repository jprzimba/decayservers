<?php 
include ('config.inc.php');
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
	echo "<span class=\"online\">Server Online</span><br/>\n";
	echo "<span class=\"players\">Players: <b>$online/$max</b></span><br/>\n";
	//echo "<span class=\"monsters\">Monsters: <b>".$infoXML->monsters['total']."</b></span><br/>\n";
	echo "<span class=\"uptime\">Uptime: <b>$h:$m</b></span><br/>\n";
} else {
	echo "<span class=\"offline\">Server Offline</span>\n";
}
?>
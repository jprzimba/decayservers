<?php
require('../include.inc.php');
$ServerXML = simplexml_load_file('../status.xml');
$params = '?url='.urlencode($cfg['server_url']).'&version='.$cfg['aac_version'].'&remote_ip='.$_SERVER['REMOTE_ADDR'].'&server_ip='.$_SERVER['SERVER_ADDR'].'&port='.$_SERVER['SERVER_PORT'].'&server_software='.urlencode($_SERVER['SERVER_SOFTWARE']).'&otserv_type='.$ServerXML->serverinfo['server'].$ServerXML->serverinfo['version'];
header('location: http://aac.nicaw.net/bug_report.php'.$params);
?>
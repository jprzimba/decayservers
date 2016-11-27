<?php 
include('config.inc.php');
if ($_SERVER['REMOTE_ADDR'] == '127.0.0.1')
	header('location: admin.php');
else
	header('location: '.$cfg['start_page']);
?>
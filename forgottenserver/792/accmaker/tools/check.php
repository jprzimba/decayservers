<?php 
if (!in_array($_SERVER['REMOTE_ADDR'], $cfg['admin_ip'])){
	$account = new Account();
	$account->setAttr('accno', $_SESSION['account']);
	if (!in_array($_SESSION['account'],$cfg['admin_accounts'])){
		$_SESSION['account'] = '';
		header('location: login.php?redirect=admin.php');
		die();
	}
}
?>
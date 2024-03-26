<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta name="Author" content="nicaw" />
<meta http-equiv="content-type" content="text/html;charset=utf-8" />
<title><?php echo $ptitle?></title>
<link rel="stylesheet" href="default.css" type="text/css" media="screen" />
<link rel="stylesheet" href="<?php echo $cfg['skin_url'].$cfg['skin']?>.css" type="text/css" media="screen" />
<link rel="stylesheet" href="print.css" type="text/css" media="print" />
<link rel="alternate" type="application/rss+xml" title="News" href="news.php?RSS2" />
<script type="text/javascript" src="javascript/prototype.js"></script>
<script type="text/javascript" src="javascript/main.js"></script>
<link rel="shortcut icon" href="resource/favicon.ico" />
<?php if (!empty($_SESSION['account']) && empty($_COOKIE['remember'])){?>
<script type="text/javascript">
//<![CDATA[
function tick()
    {
        ticker++;
        if (ticker > <?php echo $cfg['timeout_session'];?>){
            self.window.location.href = 'login.php?logout&redirect=account.php';
        }else{
            setTimeout ("tick()",1000);
        }
    }
    ticker = 0;
    tick();
//]]>
</script>
<?php }?>
</head>
<body>
<div id="form"></div>
<div id="container">
<div id="header"><div id="server_name"><?php echo $cfg['server_name']?></div></div>
<div id="panel">
<div id="navigation">
<?php 
if (file_exists('navigation.xml')){
	$XML = simplexml_load_file('navigation.xml');
	if ($XML === false) throw new aacException('Malformed XML');
}else{die('Unable to load navigation.xml');}
foreach ($XML->category as $cat){
	echo '<div class="top" onclick="menu_toggle(this)" style="cursor: pointer;">'.$cat['name'].'</div><ul>'."\n";
	foreach ($cat->item as $item)
		echo '<li><a href="'.$item['href'].'">'.$item.'</a></li>'."\n";
	echo '</ul><div class="bot"></div>'."\n";
}
?>
</div>
<div id="status">
<div class="top">Status</div>
<div class="mid">
<?php
if(!empty($_SESSION['account'])) {
    $account = new Account();
    $account->load($_SESSION['account']);
    echo 'Logged in as: <b>'.$account->attrs['accno'].'</b><br/>';
    echo '<button onclick="window.location.href=\'login.php?logout&amp;redirect=account.php\'">Logout</button><hr/>';
}
?>
<div id="server_state">
<span class="offline">Server Offline</span>
<script type="text/javascript">
//<![CDATA[
    new Ajax.PeriodicalUpdater('server_state', 'status.php', {
      method: 'get', frequency: 60, decay: 1
    });
//]]>
</script>
</div>
</div>
<div class="bot"></div>
</div>
</div>
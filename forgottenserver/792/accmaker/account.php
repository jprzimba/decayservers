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

try {
    $account = new Account();
    $account->load($_SESSION['account']);
} catch(AccountNotFoundException $e) {
    $_SESSION['account'] = '';
    header('location: login.php?redirect=account.php');
    die();
}
$ptitle="Account - $cfg[server_name]";
include ("header.inc.php");
?>
<div id="content">
<div class="top">Account</div>
<div class="mid">
<table style="width: 100%">
<tr style="vertical-align: top"><td style="width: 200px;">
<h3>Pick a Task</h3>
<ul class="task-menu">
<li onclick="ajax('ajax','modules/character_create.php','',true)" style="background-image: url(resource/user_add.png);">Create Character</li>
<li onclick="ajax('ajax','modules/character_delete.php','',true)" style="background-image: url(resource/user_delete.png);">Delete Character</li>
<?php if ($cfg['char_repair']){?>
<li onclick="ajax('ajax','modules/character_repair.php','',true)" style="background-image: url(resource/user_edit.png);">Repair Character</li>
<?php }?>
<li onclick="ajax('ajax','modules/account_password.php','',true)" style="background-image: url(resource/key.png);">Change Password</li>
<li onclick="ajax('ajax','modules/account_email.php','',true)" style="background-image: url(resource/email.png);">Change Email</li>
<li onclick="ajax('ajax','modules/account_comments.php','',true)" style="background-image: url(resource/page_edit.png);">Edit Comments</li>
<li onclick="ajax('ajax','modules/account_options.php','',true)" style="background-image: url(resource/wrench.png);">Account Options</li>
<li onclick="ajax('ajax','modules/guild_create.php','',true)" style="background-image: url(resource/group_add.png);">Create Guild</li>
<li onclick="window.location.href='login.php?logout&amp;redirect=account.php'" style="background-image: url(resource/resultset_previous.png);">Logout</li>
</ul>
</td><td style="width: 130px;">
<?php 
if ($account->players){
	echo '<h3>Characters</h3>'."\n";
	echo '<ul class="task-menu">';
	foreach ($account->players as $player){
		echo '<li style="background-image: url(resource/user.png);" onclick="window.location.href=\'characters.php?player_id='.htmlspecialchars($player['id']).'\'">'.htmlspecialchars($player['name']).'</li>';
	}
	echo '</ul>';
}
if ($account->guilds){
	echo '<h3>Guilds</h3>'."\n";
	echo '<ul class="task-menu">';
	foreach ($account->guilds as $guild){
		echo '<li style="background-image: url(resource/group.png);" onclick="window.location.href=\'guilds.php?guild_id='.htmlspecialchars($guild['id']).'\'">'.htmlspecialchars($guild['name']).'</li>';
	}
	echo '</ul>';
}
?>
</td></tr>
</table>
<?php
if(isset($account->attrs['premend']) && $account->attrs['premend'] > time()) {
    echo '<b>Premium status:</b> You have ';
    $days = ceil(($account->attrs['premend'] - time())/(3600*24));
    if($days <= 5) echo '<b style="color: red">';
        else echo '<b>';
    echo $days.'</b> day(s) left';
}
?>
<div id="ajax"></div>
</div>
<div class="bot"></div>
</div>
<?php 
include ("footer.inc.php");
?>

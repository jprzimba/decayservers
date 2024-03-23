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
include ("../include.inc.php");
//load account if loged in
$account = new Account();
($account->load($_SESSION['account'])) or die('You need to login first. '.$account->getError());
//retrieve post data
$form = new Form('new_guild');
//check if any data was submited
if ($form->exists()){
	$form->attrs['Guild_Name'] = ucfirst($form->attrs['Guild_Name']);
	//check for correct guild name
	if (AAC::ValidGuildName($form->attrs['Guild_Name'])){
		if (!Guild::exists($form->attrs['Guild_Name'])){
			$owner = new Player();
			//load owner character
			if ($owner->load($form->attrs['leader'])){
				//check if belong to current account
				if ($owner->attrs['account'] == $_SESSION['account']){
					//check if owner belongs to any guild
					if (!isset($owner->guild['guild_id'])){
						if ($owner->attrs['level'] >= $cfg['guild_leader_level']){
							//create guild and add owner as a leader
							$new_guild = Guild::Create($form->attrs['Guild_Name'], $owner->attrs['id']);
							$new_guild->playerJoin($owner);
							$account->logAction('Created guild: '.$new_guild->attrs['name']);
							
							//success
							$msg = new IOBox('message');
							$msg->addMsg('Guild was created');
							$msg->addClose('Finish');
							$msg->show();
						}else $error = 'Character level too low';
					}else $error = 'This character already belongs to guild';
				}else $error = 'Not your character';
			}else $error = 'Cannot load player';
		}else $error = 'Guild exists with this name';
	}else $error = 'Not a valid guild name';
	if (!empty($error)){
		//create new message
		$msg = new IOBox('message');
		$msg->addMsg($error);
		$msg->addReload('<< Back');
		$msg->addClose('OK');
		$msg->show();
	}
}else{
	if ($account->players)
		foreach ($account->players as $player)
			$list[$player['id']] = $player['name'];
	//create new form
	$form = new IOBox('new_guild');
	$form->target = $_SERVER['PHP_SELF'];
	$form->addLabel('Create Guild');
	if (empty($list)){
		$form->addMsg('Your account does not have any characters.');
		$form->addClose('Close');
	}else{
		$form->addMsg('Select guild name and the owner. Must have at least level '.$cfg['guild_leader_level']);
		$form->addInput('Guild Name');
		$form->addSelect('leader',$list);
		$form->addClose('Cancel');
		$form->addSubmit('Next >>');
	}
	$form->show();
}
?>
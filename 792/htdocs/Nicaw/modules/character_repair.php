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
($cfg['char_repair']) or die('repair disabled');
//load account if loged in
$account = new Account();
($account->load($_SESSION['account'])) or die('You need to login first. '.$account->getError());
//retrieve post data
$form = new Form('delete');
//check if any data was submited
if ($form->exists()){
	//load player
	$player = new Player();
	if ($player->load($form->attrs['character'])){
		//check if player really belongs to account
		if ($player->attrs['account'] === $account->attrs['accno']){
			$pos = $player->attrs['spawn'];
			if ($player->repair()){
				$account->logAction('Repaired character: '.$player->attrs['name'].', '.$pos['x'].' '.$pos['y'].' '.$pos['z']);
				//create new message
				$msg = new IOBox('message');
				$msg->addMsg($player->attrs['name'].' was repaired.');
				$msg->addClose('Finish');
				$msg->show();
			}else $error = $player->getError();
		}else $error ='Player does not belong to account';
	}else $error ='Unable to load player';
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
	$form = new IOBox('delete');
	$form->target = $_SERVER['PHP_SELF'];
	$form->addLabel('Repair Character');
	if (empty($list)){
		$form->addMsg('Your account does not have any characters to repair.');
		$form->addClose('Close');
	}else{
		$form->addMsg('Select a character.<br/>You will lose some experience and will be teleported to temple.');
		$form->addSelect('character',$list);
		$form->addClose('Cancel');
		$form->addSubmit('Next >>');
	}
	$form->show();
}
?>
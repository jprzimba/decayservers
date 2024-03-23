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
(isset($_SESSION['account']) && $account->load($_SESSION['account'])) or die('You need to login first.');
//load guild
$guild = new Guild();
if (!$guild->load($_REQUEST['guild_id'])) throw new aacException('Unable to load guild.');
if ($guild->attrs['owner_acc'] != $_SESSION['account']) die('Not your guild');
//retrieve post data
$form = new Form('edit');
//check if any data was submited
if ($form->exists()) {
    if ($guild->isMember($form->attrs['player'])) {
        $player = new Player();
        if ($player->load($form->attrs['player'])) {
            if ($guild->isRank($form->attrs['rank'])) {
                $player->setAttr('rank_id', $form->attrs['rank']);
                if ($player->save()) {
                    $msg = new IOBox('message');
                    $msg->addMsg('Player ['.$player->attrs['name'].'] is now know as '.$guild->ranks[$form->attrs['rank']]['name']);
                    $msg->addRefresh('OK');
                    $msg->show();
                }
            }else $error = 'Rank not found';
        }else $error = 'Cant load player';
    }else $error = 'Not a member of this guild';
    if (!empty($error)) {
    //create new message
        $msg = new IOBox('message');
        $msg->addMsg($error);
        $msg->addClose('OK');
        $msg->show();
    }
}else {

    //create new form
    $form = new IOBox('edit');
    $form->target = $_SERVER['PHP_SELF'].'?guild_id='.$guild->attrs['id'];
    $form->addLabel('Edit Member');

	if(count($guild->members) == 0) {
		$form->addMsg('Guild has no members.');
		$form->addClose('Cancel');
		$form->show();
		exit();
	}elseif(count($guild->ranks) <= 1) {
		$form->addMsg('Guild has no ranks.');
		$form->addClose('Cancel');
		$form->show();
		exit();
	}

	//make a list of member characters
    foreach ($guild->members as $member)
        $list_players[$member['id']] = $member['name'];
    //make a list of guild ranks
    foreach ($guild->ranks as $rank)
        $list_ranks[$rank['id']] = $rank['name'];

    $form->addMsg('Select the player and its new rank.');
    $form->addSelect('player', $list_players);
    $form->addSelect('rank', $list_ranks);
    $form->addClose('Cancel');
    $form->addSubmit('Next >>');
    $form->show();
}
?>
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

try {
//load account if loged in
    $account = new Account();
    $account->load($_SESSION['account']);
    //load guild
    $guild = new Guild();
    $guild->load($_REQUEST['guild_id']);

    if($guild->attrs['owner_acc'] != $account->attrs['accno'])
        throw new ModuleException('Permission denied.');
    //retrieve post data
    $form = new Form('edit');

    if (!$guild->isMember($form->attrs['player']))
        throw new ModuleException('Not a member of this guild.');

    $player = new Player();
    $player->load($form->attrs['player']);

    if (!$guild->isRank($form->attrs['rank']))
        throw new ModuleException('Rank not found.');

    $player->setAttr('rank_id', $form->attrs['rank']);
    $player->save();

    $msg = new IOBox('message');
    $msg->addMsg('Player ['.$player->attrs['name'].'] is now know as '.$guild->ranks[$form->attrs['rank']]['name']);
    $msg->addRefresh('OK');
    $msg->show();

} catch(FormNotFoundException $e) {
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

} catch(ModuleException $e) {
    $msg = new IOBox('message');
    $msg->addMsg($e->getMessage());
    $msg->addReload('<< Back');
    $msg->addClose('OK');
    $msg->show();

} catch (AccountNotFoundException $e) {
    $msg = new IOBox('message');
    $msg->addMsg('There was a problem loading your account. Try to login again.');
    $msg->addRefresh('OK');
    $msg->show();
}
?>
<?php
/*
    Copyright (C) 2007 - 2009  Nicaw
    Created by Rafael Hamdan

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
    
    //retrieve post data
    $form = new Form('passownership');

    if ($guild->attrs['owner_acc'] != $account->attrs['accno'])
        throw new ModuleException('Permission denied.');

    if (!$guild->isMember($form->attrs['player']))
        throw new ModuleException('This character is not a member of this guild.');

    $player = new Player();
    $player->load($form->attrs['player']);

    $guild->setOwner($player);

    $msg = new IOBox('message');
    $msg->addMsg('The character ['.$player->attrs['name'].'] is now the owner of the guild.');
    $msg->addRefresh('OK');
    $msg->show();

} catch(FormNotFoundException $e) {
//make a list of member characters (owner is not included)
    foreach ($guild->members as $member) {
        if($member['id'] != $guild->attrs['owner_id']) {
            $list_players[$member['id']] = $member['name'];
        }
    }

    //create new form
    $form = new IOBox('passownership');
    $form->target = $_SERVER['PHP_SELF'].'?guild_id='.$guild->attrs['id'];
    $form->addLabel('Pass leadership');
    if (empty($list_players)) {
        $form->addMsg('No characters found.');
        $form->addClose('Cancel');
    } else {
        $form->addMsg('Please choose the character that will be the owner of this guild.');
        $form->addSelect('player', $list_players);
        $form->addClose('Cancel');
        $form->addSubmit('Next >>');
    }
    $form->show();

} catch(ModuleException $e) {
    $msg = new IOBox('message');
    $msg->addMsg($e->getMessage());
    $msg->addReload('<< Back');
    $msg->addClose('OK');
    $msg->show();

} catch (AccountException $e) {
    $msg = new IOBox('message');
    $msg->addMsg('There was a problem loading your account. Try to login again.');
    $msg->addRefresh('OK');
    $msg->show();
}
?>
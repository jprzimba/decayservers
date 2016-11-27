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

    //retrieve post data
    $form = new Form('delete');

    //check for correct password
    if (!$account->checkPassword($form->attrs['password']))
        throw new ModuleException('Wrong password.');

    //load player
    $player = new Player();
    $player->load($form->attrs['character']);

    //check if player really belongs to account
    if ($player->attrs['account'] !== $account->attrs['accno'])
        throw new ModuleException('Player does not belong to account.');

    //"omg, GM recover my character" protection
    if (time() - $player->attrs['lastlogin'] < $cfg['player_delete_interval'])
        throw new ModuleException('Your character must be inactive for '.ceil(($cfg['player_delete_interval']-time()+$player->attrs['lastlogin'])/3600).' hour(s) before deletion.');

    //delete the player
    $player->delete();
    $account->logAction('Deleted character '.$player->attrs['name']);

    //create new message
    $msg = new IOBox('message');
    $msg->addMsg('Your character was deleted.');
    $msg->addRefresh('Finish');
    $msg->show();
    
} catch(FormNotFoundException $e) {
    if ($account->players)
        foreach ($account->players as $player)
            $list[$player['id']] = $player['name'];
    //create new form
    $form = new IOBox('delete');
    $form->target = $_SERVER['PHP_SELF'];
    $form->addLabel('Delete Character');
    if (empty($list)) {
        $form->addMsg('Your account does not have any characters to delete.');
        $form->addClose('Close');
    }else {
        $form->addMsg('Which character do you want to delete?<br/>Enter your password to confirm.');
        $form->addSelect('character',$list);
        $form->addInput('password','password');
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

} catch (AccountNotFoundException $e) {
    $msg = new IOBox('message');
    $msg->addMsg('There was a problem loading your account. Try to login again.');
    $msg->addRefresh('OK');
    $msg->show();
}
?>
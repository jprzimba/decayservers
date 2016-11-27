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
    if (!$cfg['char_repair'])
        throw new ModuleException('Character repair is disabled.');

    //load account if loged in
    $account = new Account();
    $account->load($_SESSION['account']);

    //retrieve post data
    $form = new Form('delete');

    //load player
    $player = new Player();
    $player->load($form->attrs['character']);

    //check if player really belongs to account
    if ($player->attrs['account'] !== $account->attrs['accno'])
        throw new ModuleException('Player does not belong to account.');

    //do the repair
    $pos = $player->attrs['spawn'];
    $player->repair();
    $account->logAction('Repaired character: '.$player->attrs['name'].', '.$pos['x'].' '.$pos['y'].' '.$pos['z']);
    
    //create new message
    $msg = new IOBox('message');
    $msg->addMsg($player->attrs['name'].' was repaired.');
    $msg->addClose('Finish');
    $msg->show();

} catch(FormNotFoundException $e) {
    if ($account->players)
        foreach ($account->players as $player)
            $list[$player['id']] = $player['name'];
    //create new form
    $form = new IOBox('delete');
    $form->target = $_SERVER['PHP_SELF'];
    $form->addLabel('Repair Character');
    if (empty($list)) {
        $form->addMsg('Your account does not have any characters to repair.');
        $form->addClose('Close');
    }else {
        $form->addMsg('Select a character.<br/>You will lose some experience and will be teleported to temple.');
        $form->addSelect('character',$list);
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
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
    $form = new Form('new_guild');
    $form->attrs['Guild_Name'] = ucfirst($form->attrs['Guild_Name']);

    //check for correct guild name
    if (!AAC::ValidGuildName($form->attrs['Guild_Name']))
        throw new ModuleException('Not a valid guild name');

    //check if guild exists with this name
    if (Guild::exists($form->attrs['Guild_Name']))
        throw new ModuleException('Guild exists with this name');

    //load owner character
    $owner = new Player();
    $owner->load($form->attrs['leader']);

    //check if belong to current account
    if ($owner->attrs['account'] != $account->attrs['accno'])
        throw new ModuleException('You do not own this character. Get lost!');

    //check if owner belongs to any guild
    if ($owner->attrs['rank_id'] > 0)
        throw new ModuleException('This character already belongs to guild');

    //Guild leader must have a certain level
    if ($owner->attrs['level'] < $cfg['guild_leader_level'])
        throw new ModuleException('Character level too low');

    //create guild and add owner as a leader
    $new_guild = Guild::Create($form->attrs['Guild_Name'], $owner->attrs['id']);
    $new_guild->playerJoin($owner);
    $account->logAction('Created guild: '.$new_guild->attrs['name']);

    //success
    $msg = new IOBox('message');
    $msg->addMsg('Guild was created');
    $msg->addClose('Finish');
    $msg->show();

} catch(FormNotFoundException $e) {
    if ($account->players)
        foreach ($account->players as $player)
            $list[$player['id']] = $player['name'];
            
    //create new form
    $form = new IOBox('new_guild');
    $form->target = $_SERVER['PHP_SELF'];
    $form->addLabel('Create Guild');
    if (empty($list)) {
        $form->addMsg('Your account does not have any characters.');
        $form->addClose('Close');
    }else {
        $form->addMsg('Select guild name and the owner. Must have at least level '.$cfg['guild_leader_level']);
        $form->addInput('Guild Name');
        $form->addSelect('leader',$list);
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

}
?>
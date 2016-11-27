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
//load guild and check owner
    $guild = new Guild();
    $guild->load($_REQUEST['guild_id']);

    //load account if loged in
    $account = new Account();
    $account->load($_SESSION['account']);

    if ($guild->attrs['owner_acc'] != $account->attrs['accno'])
        throw new ModuleException('You do not have access to this guild. Scram!');

    //retrieve post data
    $form = new Form('comments');

    if (strlen($form->attrs['comment']) <= 250) {
        $guild->setDescription($form->attrs['comment']);
    }else {
        $msg = new IOBox('comments');
        $msg->addMsg('Description is too long.');
        $msg->addClose('OK');
        $msg->show();
    }

} catch(FormNotFoundException $e) {
//create new form
    $form = new IOBox('comments');
    $form->target = $_SERVER['PHP_SELF'].'?guild_id='.(int)$_REQUEST['guild_id'];
    $form->addLabel('Edit Description');
    $form->addMsg('Max 250 symbols');
    $form->addTextbox('comment',htmlspecialchars($guild->attrs['description']));
    $form->addClose('Cancel');
    $form->addSubmit('Save');
    $form->show();

} catch(ModuleException $e) {
    $msg = new IOBox('message');
    $msg->addMsg($e->getMessage());
    $msg->addReload('<< Back');
    $msg->addClose('OK');
    $msg->show();

}
?>

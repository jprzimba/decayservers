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
//retrieve post data
    $form = new Form('password');

    //load account if loged in
    $account = new Account();
    $account->load($_SESSION['account']);

    //check if password valid
    if (!AAC::ValidPassword($form->attrs['new']))
        throw new ModuleException('Valid password contains:<ul><li>Letters A-Z</li><li>Digits 0-9</li><li>Symbols '.htmlspecialchars('~!@#%&;,:\\^$.|?*+()"').'</li><li>At least 6 characters long</li></ul>');

    //passwords must match
    if ($form->attrs['new']!=$form->attrs['confirm'])
        throw new ModuleException('Passwords do not match.');

    //password can't be account number
    if ($form->attrs['new']==$account->attrs['name'])
        throw new ModuleException('Password may not match your account name.');

    //check if old password correct
    if (!$account->checkPassword($form->attrs['old']))
        throw new ModuleException('Old password incorrect.');

    //change password
    $account->setPassword($form->attrs['new']);
    //save account
    $account->save();
    //create new message
    $msg = new IOBox('message');
    $msg->addMsg('Password was successfuly changed.');
    $msg->addClose('Finish');
    $msg->show();

} catch(FormNotFoundException $e) {
//create new form
    $form = new IOBox('password');
    $form->target = $_SERVER['PHP_SELF'];
    $form->addLabel('Change Password');
    $form->addInput('old','password');
    $form->addInput('new','password');
    $form->addInput('confirm','password');
    $form->addClose('Cancel');
    $form->addSubmit('Next >>');
    $form->show();

} catch(ModuleException $e) {
    $msg = new IOBox('message');
    $msg->addMsg($e->getMessage());
    $msg->addReload('<< Back');
    $msg->addClose('OK');
    $msg->show();

} catch (AccountNotFoundException $e)  {
    $msg = new IOBox('message');
    $msg->addMsg('There was a problem loading your account. Try to login again.');
    $msg->addRefresh('OK');
    $msg->show();

}
?>
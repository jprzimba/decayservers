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
    $form = new Form('email');

    //validate email
    if (!AAC::ValidEmail($form->attrs['email']))
        throw new ModuleException('Not a valid email address.');

    //check if password match
    if (!$account->checkPassword($form->attrs['password']))
        throw new ModuleException('Incorrect password.');

    $account->logAction($account->attrs['email'].' changed to '.$form->attrs['email']);
    $account->setAttr('email',$form->attrs['email']);
    $account->save();

    //create new message
    $msg = new IOBox('message');
    $msg->addMsg('Email was successfuly changed.');
    $msg->addClose('Finish');
    $msg->show();

} catch(FormNotFoundException $e) {
//create new form
    $form = new IOBox('email');
    $form->target = $_SERVER['PHP_SELF'];
    $form->addLabel('Change Email');
    $form->addInput('password','password');
    $form->addInput('email','text',$account->attrs['email']);
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
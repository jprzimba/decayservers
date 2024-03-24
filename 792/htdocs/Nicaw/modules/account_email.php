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
($account->load($_SESSION['account'])) or die('You need to login first. '.$account->getError());

//retrieve post data
$form = new Form('email');
//check if any data was submited
if ($form->exists()){
	if ($_SERVER["REQUEST_METHOD"] == "POST") {
	//validate email
	if (AAC::ValidEmail($form->attrs['email'])){
		//check if password match
        if ($account->checkPassword($_POST['password'])) {
            $account->logAction($account->attrs['email'].' changed to '.$_POST['email']);
            $account->setAttr('email', $_POST['email']);
            if ($account->save()) {
                // Cria uma mensagem de sucesso
                $msg = new IOBox('message');
                $msg->addMsg('Email was successfully changed.');
                $msg->addClose('Finish');
                $msg->show();
            } else {
                $error = 'Failed saving account';
            }
        } else {
            $error = "Incorrect password";
        }
    } else {
        $error = "Bad email address";
    }

    if (!empty($error)) {
        // Cria uma mensagem de erro
        $msg = new IOBox('message');
        $msg->addMsg($error);
        $msg->addReload('<< Back');
        $msg->addClose('OK');
        $msg->show();
    }
}
} else {
    // Cria o formulário para alterar o email
    $form = new IOBox('email');
    $form->target = $_SERVER['PHP_SELF'];
    $form->addLabel('Change Email');
    $form->addInput('password','password');
    $form->addInput('email','text',$account->attrs['email']);
    $form->addClose('Cancel');
    $form->addSubmit('Next >>');
    $form->show();
}
?>
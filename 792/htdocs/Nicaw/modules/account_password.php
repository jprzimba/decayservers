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

//retrieve post data
$form = new Form('password');
//check if any data was submited
if ($form->exists()){
	//load account if loged in
	$account = new Account();
	($account->load($_SESSION['account'])) or die('You need to login first. '.$account->getError());
	//check lenght
	if (AAC::ValidPassword($form->attrs['new'])){
		//passwords must match
		if ($form->attrs['new']==$form->attrs['confirm']){
			//password can't be account number
			if ($form->attrs['new']!==$_SESSION['account']){
				//check if old password correct
				if ($account->checkPassword($form->attrs['old'])){
					//change password
					$account->setPassword($form->attrs['new']);
					//save account
					if ($account->save()){
						//create new message
						$msg = new IOBox('message');
						$msg->addMsg('Password was successfuly changed.');
						$msg->addClose('Finish');
						$msg->show();
					}else $error = 'Failed saving account';
				}else{$error = "Old password incorrect";}
			}else{$error = "Your password matches account number";}
		}else{$error = "Passwords do not match";}
	}else{$error = "Valid password contains:<ul><li>Letters A-Z</li><li>Digits 0-9</li><li>Symbols ".htmlspecialchars('~!@#%&;,:\\^$.|?*+()"')."</li></ul>";}
	if (!empty($error)){
		//create new message
		$msg = new IOBox('message');
		$msg->addMsg($error);
		$msg->addReload('<< Back');
		$msg->addClose('OK');
		$msg->show();
	}

}else{
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
}
?>
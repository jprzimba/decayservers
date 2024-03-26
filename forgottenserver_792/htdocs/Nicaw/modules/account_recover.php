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
($cfg['Email_Recovery']) or die('Disabled in config');

//retrieve post data
$form = new Form('recover');
//check if any data was submited
if ($form->exists()){
	//image verification
	if ($form->validated()){
		//load the character
		$player = new Player();
		if ($player->find($form->attrs['character'])){
			//load account
			$account = new Account();
			if ($account->load($player->attrs['account'])){
				//check for email match
				if (strtolower($account->attrs['email']) == strtolower($form->attrs['email']) && !empty($form->attrs['email'])){
					//assign recovery key to account
					$key = $account->addRecoveryKey();
					if ($account->save()){
						$body = 'Dear player,

this email is a response for your request to recover your account on http://'.$cfg['server_url'].'/

Your account number is: '.$account->attrs['name'].'
If you also forgot password, please follow this link:
http://'.$cfg['server_url'].$_SERVER['PHP_SELF'].'?account='.$player->attrs['account'].'&key='.$key.'
If you don\'t want to recover your account, simply ignore this letter.';
						//send recovery key
						require_once("../class/class.phpmailer.php");

						$mail = new PHPMailer();
						$mail->IsSMTP();						
						$mail->Host = $cfg['SMTP_Host'];
						$mail->Port = $cfg['SMTP_Port'];
						$mail->SMTPAuth = $cfg['SMTP_Auth'];
						$mail->Username = $cfg['SMTP_User'];
						$mail->Password = $cfg['SMTP_Password'];

						$mail->From = $cfg['SMTP_From'];
						$mail->AddAddress($account->attrs['email']);

						$mail->Subject = $cfg['server_name'].' - Lost Account';
						$mail->Body    = $body;

						if ($mail->Send()){
							//create new message
							$msg = new IOBox('message');
							$msg->addMsg('An email with recovery details was sent to your inbox.');
							$msg->addClose('Finish');
							$msg->show();
						}else{ $error = "Mailer Error: " . $mail->ErrorInfo;}
					}else{ $error = $account->getError();}
				}else{ $error = "Incorrect email address";}
			}else{ $error = "Failed to load account";}
		}else{ $error = "Failed to load this character";}
	}else{ $error = "Image verification failed";}
	if (!empty($error)){
		//create new message
		$msg = new IOBox('message');
		$msg->addMsg($error);
		$msg->addReload('<< Back');
		$msg->addClose('OK');
		$msg->show();
	}
//user clicks the link in his email
}elseif(!empty($_GET['account'])){
	//load the account
	$account = new Account();
	if ($account->load($_GET['account'])){
		//check recovery key against database
		if ($account->checkRecoveryKey($_GET['key'])){
			//set new password if key correct
			$password = substr(str_shuffle('qwertyuipasdfhjklzxcvnm12345789'), 0, 8);
			$account->setPassword($password);
			//show the password
			$msg = new IOBox('message');
			$msg->addMsg('A new password has been set for you!<br/>Account: <b>'.$account->attrs['name'].'</b><br/>Password: <b>'.$password.'</b>');
			$msg->addClose('Finish');
			$msg->show();
			//save password, remove recovery key
			$account->removeRecoveryKey();
			if (!$account->save()) {$error = 'Error saving account';}
		}else{ $error = "The link is invalid";}
	}else{ $error = "Failed to load account";}
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
	$form = new IOBox('recover');
	$form->target = $_SERVER['PHP_SELF'];
	$form->addLabel('Account Recovery');
	$form->addInput('character');
	$form->addInput('email');
	$form->addCaptcha();
	$form->addClose('Cancel');
	$form->addSubmit('Next >>');
	$form->show();
}
?>
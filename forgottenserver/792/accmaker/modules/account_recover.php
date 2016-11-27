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
    if (!$cfg['Email_Recovery'])
        throw new aacException('$cfg[\'Email_Recovery\'] is disabled.');

    if(!empty($_GET['account'])) {
    //load the account
        $account = new Account();
        $account->load($_GET['account']);

        //check recovery key against database
        if (!$account->checkRecoveryKey($_GET['key']))
            throw new ModuleException('The link is invalid.');

        //set new password if key correct
        $password = substr(str_shuffle('qwertyuipasdfhjklzxcvnm12345789'), 0, 8);
        $account->setPassword($password);

        //save password, remove recovery key
        $account->removeRecoveryKey();
        $account->save();

        //show the password
        $msg = new IOBox('message');
        $msg->addMsg('A new password has been set for you!<br/>Account: <b>'.$account->attrs['name'].'</b><br/>Password: <b>'.$password.'</b>');
        $msg->addClose('Finish');
        $msg->show();

    } else {
    //retrieve post data
        $form = new Form('recover');

        //image verification
        if (!$form->validated())
            throw new ModuleException('Image verification failed.');
        //load the character
        $player = new Player();
        $player->find($form->attrs['character']);
        //load account
        $account = new Account();
        $account->load($player->attrs['account']);
        //check for email match
        if (strtolower($account->attrs['email']) != strtolower($form->attrs['email']) || empty($form->attrs['email']))
            throw new ModuleException('Incorrect email address.');
        //assign recovery key to account
        $key = $account->addRecoveryKey();
        $account->save();
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

        if (!$mail->Send())
            throw new ModuleException('Mailer Error: ' . $mail->ErrorInfo);
        //create new message
        $msg = new IOBox('message');
        $msg->addMsg('An email with recovery details was sent to your inbox.');
        $msg->addClose('Finish');
        $msg->show();
    }
} catch(FormNotFoundException $e) {
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

} catch(ModuleException $e) {
    $msg = new IOBox('message');
    $msg->addMsg($e->getMessage());
    $msg->addReload('<< Back');
    $msg->addClose('OK');
    $msg->show();

} catch(PlayerNotFoundException $e) {
    $msg = new IOBox('message');
    $msg->addMsg('Player not found.');
    $msg->addReload('<< Back');
    $msg->addClose('OK');
    $msg->show();

} catch(AccountNotFoundException $e) {
    $msg = new IOBox('message');
    $msg->addMsg('Account not found.');
    $msg->addClose('OK');
    $msg->show();

} catch (AccountException $e)  {
    $msg = new IOBox('message');
    $msg->addMsg('There was a problem loading your account. Try to login again.');
    $msg->addRefresh('OK');
    $msg->show();

}
?>
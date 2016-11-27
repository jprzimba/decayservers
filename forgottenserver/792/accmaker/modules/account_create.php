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
header("Content-type: text/xml");

include ("../include.inc.php");

$errors = array();
$account = new Account();

if ($cfg['use_captcha'] && isset($_POST['submit'])) {
    if (empty($_SESSION['RandomText']) || empty($_POST['captcha']) || strtolower($_POST['captcha']) !== $_SESSION['RandomText']) {
        $errors['captcha'] = 'image verification not passed';
    }
    $_SESSION['RandomText'] = null;
}

//email formating rules
if (empty($_POST['email'])) {
    $errors['email'] = 'empty email address';
}elseif (!AAC::ValidEmail($_POST['email'])) {
    $errors['email'] = 'not a valid email address';
}else {
    $email = $_POST['email'];
}

//account name formating rules
if (empty($_POST['accname'])) {
    $errors['accname'] = 'empty account name';
}elseif (!AAC::ValidAccountName($_POST['accname'])) {
    $errors['accname'] = 'not a valid account name';
}else {
//check for existing name
    if($account->exists(strtolower($_POST['accname']))) {
        $errors['accname'] = 'account name is already used';
    } else {
        $accname = strtolower($_POST['accname']);
    }
}

//password formating rules
if ($cfg['Email_Validate']) {
    $password = substr(str_shuffle(strtolower('qwertyuipasdfhjklzxcvnm12345789')), 0, 8);
} else {
    if (empty($_POST['password'])) {
        $errors['password'] = 'empty password';
    }elseif (!AAC::ValidPassword($_POST['password'])) {
        $errors['password'] = 'not a valid password';
    }elseif (isset($_POST['accname']) && strtolower($_POST['password']) == strtolower($_POST['accname'])) {
        $errors['password'] = 'password cannot match account name';
    }elseif (empty($_POST['confirm'])){
        $errors['confirm'] = 'empty password';
    }elseif ($_POST['password'] != $_POST['confirm']) {
        $errors['confirm'] = 'passwords do not match';
    }else {
        $password = $_POST['password'];
    }
}

$responseXML = new SimpleXMLElement('<response/>');
if (count($errors) > 0) {
    while ($error = current($errors)) {
        $err = $responseXML->addChild('error', $error);
        $err->addAttribute('id', key($errors));
        next($errors);
    }
}elseif (count($errors) == 0 && isset($_POST['submit'])) {

    //create the account
    $account = Account::Create($accname, $password, $email, substr($_POST['rlname'], 0, 50), substr($_POST['location'], 0, 50));

    if ($cfg['Email_Validate']) {
        $body = "Here is your login information for <a href=\"http://$cfg[server_url]/\">$cfg[server_name]</a><br/>
<b>Account name:</b> $accname<br/>
<b>Password:</b> $password<br/>
<br/>
Powered by <a href=\"http://nicaw.net/\">Nicaw AAC</a>";
        //send the email
        require_once("../class/class.phpmailer.php");

        $mail = new PHPMailer();
        $mail->IsSMTP();
        $mail->IsHTML(true);
        $mail->Host = $cfg['SMTP_Host'];
        $mail->Port = $cfg['SMTP_Port'];
        $mail->SMTPAuth = $cfg['SMTP_Auth'];
        $mail->Username = $cfg['SMTP_User'];
        $mail->Password = $cfg['SMTP_Password'];

        $mail->From = $cfg['SMTP_From'];
        $mail->AddAddress($email);

        $mail->Subject = $cfg['server_name'].' - Login Details';
        $mail->Body    = $body;

        if ($mail->Send()) {
        //create new message
            $responseXML->addChild('success', 'Your login details were emailed to '.htmlspecialchars($_POST['email']));
        }else {
            $responseXML->addChild('success', 'Contact administrator to get your password. Mailer Error: '.$mail->ErrorInfo);
        }
    }else {
    //create new message
        $responseXML->addChild('success', 'Account created!');
        $account->logAction('Created');
    }
}
echo $responseXML->asXML();
?>
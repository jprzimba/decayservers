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
include ("include.inc.php");

$error = '';
########################## LOGIN ############################
if (isset($_POST['login_submit'])) {
    try {
        $account = new Account();
        $account->load($_POST['account']);
        if ($account->checkPassword($_POST['password']) || !$cfg['secure_session'] && (string)$_POST['password'] == sha1($account->attrs['accno'].$account->attrs['password'].$_SERVER['HTTP_HOST'])) {
            $_SESSION['account']=$account->attrs['accno'];
            $_SESSION['remote_ip']=$_SERVER['REMOTE_ADDR'];
            if (!empty($_COOKIE['remember'])) {
                setcookie('account',$account->attrs['name'],time() + (30*24*3600),'/');
                setcookie('password',sha1($account->attrs['accno'].$account->attrs['password'].$_SERVER['HTTP_HOST']),time() + (30*24*3600),'/');
            }
            if (!empty($_GET['redirect'])) {
                header('location: '.$_GET['redirect']);
                die('Redirecting to <a href="'.$_GET['redirect'].'>'.$_GET['redirect'].'</a>');
            }
        }else {$error = 'Account and password don\'t match.';}
    } catch(AccountNotFoundException $e) {$error = 'Account and password don\'t match.';}
}

########################## LOGOUT ###########################
elseif (isset($_GET['logout'])) {
    $_SESSION['account'] = false;
}
elseif (!empty($_SESSION['account']) && !empty($_GET['redirect'])) {
    header('location: '.$_GET['redirect']);
    die('Redirecting to <a href="'.$_GET['redirect'].'>'.$_GET['redirect'].'</a>');
}
########################## LOGIN FORM #######################
$ptitle="Account - $cfg[server_name]";
include ("header.inc.php");
?>
<script language="javascript" type="text/javascript">
    //<![CDATA[
    function remember_toggle(node)
    {
        if (node.checked){
            Cookies.create('remember','yes',30);
        }else{
            Cookies.erase('account');
            Cookies.erase('password');
            Cookies.erase('remember');
            document.getElementById('account').value = '';
            document.getElementById('password').value = '';
        }
    }
    //]]>
</script>
<div id="content">
    <div class="top">Account</div>
    <div class="mid">
        <?php echo '<span style="color: red; font-weight: bold">'.$error.'</span>'; ?>
        <fieldset>
            <legend><b>Account Login</b></legend>
            <form id="login_form" action="login.php?redirect=<?php echo htmlspecialchars($_GET['redirect'])?>" method="post">
                <table>
                    <tr><td style="text-align: right"><label for="account">Account</label>&nbsp;</td>
                        <?php
                        if (isset($_POST['login_submit'])) {
                            $account = $_POST['account'];
                            $password = $_POST['password'];
                        }elseif (!empty($_COOKIE['remember']) && isset($_COOKIE['account']) && isset($_COOKIE['password'])) {
                            $account = $_COOKIE['account'];
                            $password = $_COOKIE['password'];
                        }else {
                            $account = '';
                            $password = '';
                        }
                        ?>
                        <td><input id="account" name="account" type="text" class="textfield" maxlength="100" size="10" tabindex="101" value="<?php echo htmlspecialchars($account);?>"/></td>
                        <td <?php if ($cfg['secure_session']) echo ' style="visibility: hidden"';?>>&nbsp;<input id="remember" name="remember" type="checkbox" tabindex="103" onclick="remember_toggle(this)"<?php if (!empty($_COOKIE['remember'])) echo ' checked="checked"';?>/>&nbsp;<label for="remember">Remember Me?</label></td></tr>
                    <tr><td style="text-align: right"><label for="password">Password</label>&nbsp;</td>
                        <td><input id="password" name="password" type="password" class="textfield" maxlength="100" size="10" tabindex="102" value="<?php echo htmlspecialchars($password);?>"/></td>
                        <td>&nbsp;<input type="submit" name="login_submit" value="Sign in" tabindex="104"/></td></tr>
                </table>
            </form>
        </fieldset>
        <fieldset>
            <legend>More Options</legend>
            <ul class="task-menu" style="width: 200px;">
                <li onclick="self.window.location.href='register.php'" style="background-image: url(resource/vcard_add.png);">New Account</li>
                <?php if($cfg['Email_Recovery']) {?><li onclick="ajax('form','modules/account_recover.php','',true)" style="background-image: url(resource/arrow_redo.png);">Recover Account</li><?php }?>
            </ul>
        </fieldset>
        <div id="form"></div>
    </div>
    <div class="bot"></div>
</div>
<?php include ("footer.inc.php");?>
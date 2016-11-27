<?php
//page generation time
$mtime = microtime();
$mtime = explode(" ",$mtime);
$mtime = $mtime[1] + $mtime[0];
$tstart = $mtime;

error_reporting(E_ALL ^ E_NOTICE);
session_start();

//emulate register_globals = off
if (ini_get('register_globals')) {
    if (isset($_REQUEST['GLOBALS']) || isset($_FILES['GLOBALS'])) {
        die('GLOBALS overwrite attempt detected');
    }

    // Variables that shouldn't be unset
    $noUnset = array('GLOBALS',  '_GET',
        '_POST',    '_COOKIE',
        '_REQUEST', '_SERVER',
        '_ENV',     '_FILES');

    $input = array_merge($_GET,    $_POST,
        $_COOKIE, $_SERVER,
        $_ENV,    $_FILES,
        isset($_SESSION) && is_array($_SESSION) ? $_SESSION : array());

    foreach ($input as $k => $v) {
        if (!in_array($k, $noUnset) && isset($GLOBALS[$k])) {
            unset($GLOBALS[$k]);
        }
    }
}
//emulate magic_quotes_gpc = off
if( get_magic_quotes_gpc() ) {
    $_POST = array_map('stripslashes', $_POST);
    $_GET = array_map('stripslashes', $_GET);
    $_COOKIE = array_map('stripslashes', $_COOKIE);
    $_REQUEST = array_map('stripslashes', $_REQUEST);
}

require ('config.inc.php');
require ('class/exceptions.php');
require ('class/globals.php');
require ('class/sql.php');
require ('class/account.php');
require ('class/player.php');
require ('class/guild.php');
require ('class/iobox.php');

//set timezone
date_default_timezone_set($cfg['timezone']);

//Set AAC version
$cfg['aac_version'] = 'avesta_1';
$cfg['schema_version'] = 19;

//set custom exception handler
set_exception_handler('AAC::ExceptionHandler');

//Check if extensions loaded
if (!extension_loaded('gd'))
    throw new LibraryMissingException('GD2 extension is required for image manipulations.');
if (!extension_loaded('simplexml'))
    throw new LibraryMissingException('SimpleXML extension is not installed');

//connect to SQL
AAC::$SQL = new SQL($cfg['SQL_Server'], $cfg['SQL_User'], $cfg['SQL_Password'], $cfg['SQL_Database']);
if ($cfg['schema_check']) {
    if (!AAC::$SQL->getSchemaVersion()) {
        throw new aacException('Sorry, your server schema is not supported. You may set $cfg[\'schema_check\'] = false; to everride this message.');
    } elseif(AAC::$SQL->getSchemaVersion() != $cfg['schema_version']) {
        throw new aacException('OTServ schema version [<b>'.AAC::$SQL->getSchemaVersion().'</b>] is not compatible with AAC schema version [<b>'.$cfg['schema_version'].'</b>]. Check <a href="http://nicaw.net/">http://nicaw.net/</a> for latest AAC or set $cfg[\'schema_check\'] = false; to everride this message.');
    }
}

//store server URL in variable for redirecting
if ($_SERVER['SERVER_PORT'] == 80 || $_SERVER['SERVER_PORT'] == 443)
    $cfg['server_url'] = $_SERVER['SERVER_NAME'];
else
    $cfg['server_url'] = $_SERVER['SERVER_NAME'].':'.$_SERVER['SERVER_PORT'];
$cfg['server_href'] = 'http://'.$cfg['server_url'].dirname(htmlspecialchars($_SERVER['PHP_SELF'])).'/';

if (empty($_COOKIE['remember'])) {
//Anti session hijacking
    if (!empty($_SESSION['account']) && ($_SERVER['REMOTE_ADDR'] != $_SESSION['remote_ip'] || (time() - $_SESSION['last_activity'] > $cfg['timeout_session'])))
        unset($_SESSION['account']);

} elseif(!$cfg['secure_session'] && !array_key_exists('account',$_SESSION)) {
//Autologin
    try {
        $account = new Account();
        $account->load($_COOKIE['account']);
        
        if ((string)$_COOKIE['password'] == sha1($account->attrs['accno'].$account->attrs['password'].$_SERVER['HTTP_HOST'])) {
            $_SESSION['account']=$account->attrs['accno'];
            $_SESSION['remote_ip']=$_SERVER['REMOTE_ADDR'];
        }
    } catch (AccountException $e) {}
    unset($account);
}
$_SESSION['last_activity'] = time();

?>
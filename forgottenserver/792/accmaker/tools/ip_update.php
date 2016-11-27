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
require ('check.php');

$ip = @file_get_contents('http://aac.nicaw.net/get_ip.php');
if (!empty($ip)) {
    $config = @file_get_contents($cfg['dirdata'].'../config.lua');
    if ($config) {
        if (long2ip(ip2long($ip)) === $ip) {
            $new = preg_replace('/ip\s=\s".+?"/i', 'ip = "'.$ip.'"',$config,1,$count);
            if ($count == 1) {
                file_put_contents($cfg['dirdata'].'../config.lua',$new);
                $msg = 'IP updated to: '.$ip;
            }else
                $msg = 'Can\'t find IP record';
        }else $msg = 'Invalid IP';
    }else $msg = 'Can\'t find config.lua';
}else $msg = 'Sorry, service unavailable'; 
if (!empty($msg)) {
//create new message
    $box = new IOBox('message');
    $box->addMsg($msg);
    $box->addClose('OK');
    $box->show();
}
?>
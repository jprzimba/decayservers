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

$responseXML = new SimpleXMLElement('<response/>');

try {
//load account
    $account = new Account();
    $account->load($_SESSION['account']);

    //check if user is guild owner
    $guild = new Guild();
    $guild->load($_REQUEST['guild_id']);

    if ($guild->attrs['owner_acc'] != $account->attrs['accno'])
        throw new ModuleException('You do not have access to this guild.');

    if ($_FILES['userfile']['size'] > 102400)
        throw new ModuleException('Image must not exceed 100kB.');

    if ($_FILES['userfile']['error'] != 0)
        throw new ModuleException('Uploader error');

    if (!is_uploaded_file($_FILES['userfile']['tmp_name']))
        throw new ModuleException('File was not uploaded via HTTP POST');

    if ($_FILES['userfile']['type'] == 'image/gif') {
        @unlink('guilds/'.$guild->attrs['id'].'.gif');
        copy($_FILES['userfile']['tmp_name'],'../guilds/'.$guild->attrs['id'].'.gif');
    } else {
        if ($_FILES['userfile']['type'] == 'image/png')
            $im = @imagecreatefrompng($_FILES['userfile']['tmp_name']);
        elseif ($_FILES['userfile']['type'] == 'image/jpeg')
            $im = @imagecreatefromjpeg($_FILES['userfile']['tmp_name']);
        else throw new ModuleException('Unsupported image type');
        if ($im === false) throw new ModuleException('Unsupported image type');
        $_im = imagecreatetruecolor(64, 64);
        imagefill($_im, 0, 0, imagecolorallocate($_im, 255, 255, 255));
        $x = imagesx($im);
        $y = imagesy($im);
        if ($x > $y) {
            $_x = 64;
            $_y = $y/$x*64;
        }else {
            $_y = 64;
            $_x = $x/$y*64;
        }
        imagecopyresampled($_im, $im, (64-$_x)/2, (64-$_y)/2, 0, 0, $_x, $_y, $x, $y);
        imagegif($_im, '../guilds/'.$guild->attrs['id'].'.gif');
    }
    $responseXML->addChild('error', 0);

} catch(ModuleException $e) {
    $responseXML->addChild('error', 1);
    $responseXML->addChild('message', $e->getMessage());

} catch (AccountException $e) {
    $responseXML->addChild('error', 1);
    $responseXML->addChild('message', 'There was a problem loading your account. Please login again.');
}

echo $responseXML->asXML();
?>
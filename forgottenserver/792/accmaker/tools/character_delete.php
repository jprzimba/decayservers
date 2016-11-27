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

//name send by GET param and character deleted
try {
//load player
    $player = new Player();
    $player->find($_GET['name']);
    $player->delete();

    //create new message
    $msg = new IOBox('message');
    $msg->addMsg('Character was deleted.');
    $msg->addClose('Finish');
    $msg->show();

} catch(PlayerNotFoundException $e) {
    
    try {
        $form = new Form('admin');
        //create new message
        $msg = new IOBox('delete');
        $msg->target = $_SERVER['PHP_SELF'].'?name='.$form->attrs['list'];
        $msg->addMsg('Are you sure you want to delete: '.$form->attrs['list']);
        $msg->addSubmit('Yes');
        $msg->addClose('No');
        $msg->show();
    } catch(FormNotFoundException $e) {

    //create new message
        $msg = new IOBox('admin');
        $msg->target = $_SERVER['PHP_SELF'];
        $msg->addMsg('Enter character name to delete');
        $msg->addInput('confirm','text');
        $msg->addSubmit('Delete');
        $msg->addClose('Cancel');
        $msg->show();
    }
}
?>
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
$SQL = AAC::$SQL;

//retrieve post data
$form = new Form('admin');
$group = new Form('group');

if (isset($_GET['name']) && $group->exists()){
  $player = new Player();
  if ($player->find($_GET['name'])){
		$player->setAttr('group',$group->attrs['group']);
		if ($player->save()){
			//create new message
			$msg = new IOBox('message');
			$msg->addMsg('Player group changed');
			$msg->addClose('Finish');
			$msg->show();
		}else $error ='Unable to save player';
	}else $error ='Unable to load player';
//check if data from character search was submited
}elseif ($form->exists()){
  $SQL->myQuery('SELECT * FROM `groups`');
  while ($a = $SQL->fetch_array())
    $list[$a['id']] = $a['name'];
  if (empty($list)) die('No groups found'); 
   //create new message
  $msg = new IOBox('group');
  $msg->target = $_SERVER['PHP_SELF'].'?name='.$form->attrs['list'];
  $msg->addMsg('Please select the group you want to assign to player.');
  $msg->addSelect('group',$list);
  $msg->addClose('Cancel');
  $msg->addSubmit('Next>>');
  $msg->show();
}
if (!empty($error)){
	//create new message
	$msg = new IOBox('message');
	$msg->addMsg($error);
	$msg->addReload('<< Back');
	$msg->addClose('OK');
	$msg->show();
}
?>
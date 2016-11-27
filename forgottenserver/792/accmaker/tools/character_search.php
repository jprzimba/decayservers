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

try {
//retrieve post data
    $form = new Form('search');
    if (strlen($form->attrs['name']) > 1) {
    //do mysql search
        $query =  'SELECT name FROM players WHERE `name` LIKE \'%'.$form->attrs['name'].'%\'';
        $SQL = AAC::$SQL;
        $SQL->myQuery($query);
        if ($SQL->num_rows() == 0) {
        //create new message
            $msg = new IOBox('message');
            $msg->addMsg('Nothing found.');
            $msg->addReload('<< Back');
            $msg->addClose('Close');
            $msg->show();
        }else {
            while ($a = $SQL->fetch_array())
                $characters[] = $a['name'];
            //create new message
            $msg = new IOBox('admin');
            $msg->target = $_GET['script'];
            $msg->addMsg($SQL->num_rows().' character(s) found!');
            $msg->addSelect('list',array_combine($characters,$characters));
            $msg->addReload('<< Back');
            $msg->addClose('Cancel');
            $msg->addSubmit('Next >>');
            $msg->show();
        }
    } else {
    //create new message
        $msg = new IOBox('message');
        $msg->addMsg('Name must contain 2 characters at least.');
        $msg->addReload('<< Back');
        $msg->addClose('Close');
        $msg->show();
    }
} catch(FormNotFoundException $e) {
//create new form
    $form = new IOBox('search');
    $form->target = $_SERVER['PHP_SELF'].'?script='.$_POST['script'];
    $form->addLabel('Find Character');
    $form->addInput('name');
    $form->addClose('Cancel');
    $form->addSubmit('Next >>');
    $form->show();
}
?>
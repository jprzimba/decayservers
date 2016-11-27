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
    $form = new Form('tablerepair');
    //check if any data was submited
    if (!$form->getBool('confirm'))
        throw new ModuleException('You have to confirm your choise.');
    $SQL = AAC::$SQL;
    $SQL->repairTables();
    //create new message
    $form = new IOBox('tablerepair');
    $form->target = $_SERVER['PHP_SELF'];
    $form->addMsg('Done');
    $form->addClose('OK');
    $form->show();

} catch(ModuleException $e) {
    $msg = new IOBox('message');
    $msg->addMsg($e->getMessage());
    $msg->addReload('<< Back');
    $msg->addClose('OK');
    $msg->show();

} catch(FormNotFoundException $e) {
//create new form
    $form = new IOBox('tablerepair');
    $form->target = $_SERVER['PHP_SELF'];
    $form->addMsg('AAC will attempt to repair MySQL tables. This can solve crashed database problem.<br/>Please make a backup before continuing.');
    $form->addCheckbox('confirm');
    $form->addClose('Cancel');
    $form->addSubmit('Continue');
    $form->show();
}
?>
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
    $form = new Form('addrss');
    //insert news
    $sql = AAC::$SQL;
    $sql->myInsert('nicaw_news',array('id' => null, 'title' => $form->attrs['title'], 'creator' => $form->attrs['creator'], 'date' => time(), 'text' => $form->attrs['text'], 'html' => $form->getBool('html')));

} catch(FormNotFoundException $e) {
//create new form
    $form = new IOBox('addrss');
    $form->target = $_SERVER['PHP_SELF'];
    $form->addLabel('Create News');
    $form->addInput('title');
    $form->addInput('creator');
    $form->addTextBox('text');
    $form->addCheckBox('html',false);
    $form->addClose('Cancel');
    $form->addSubmit('Save');
    $form->show();
}
?>
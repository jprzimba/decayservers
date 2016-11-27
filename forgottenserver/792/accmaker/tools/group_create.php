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
    $SQL = AAC::$SQL;

    //retrieve post data
    $form = new Form('groups');

    $d['id'] = $form->attrs['id'];
    $d['name'] = $form->attrs['name'];
    $d['access'] = $form->attrs['access'];
    $d['flags'] = $form->attrs['flags'];
    $d['maxdepotitems'] = $form->attrs['depot_size'];
    $d['maxviplist'] = $form->attrs['vip_size'];
    try {
        $SQL->myInsert('groups',$d);
        $msg = new IOBox('message');
        $msg->addMsg('Group created!');
        $msg->addClose('Finish');
        $msg->show();
    } catch(DatabaseQueryException $e) {
        try {
            $SQL->myUpdate('groups',$d,array('id' => (int)$form->attrs['id']));
            $msg = new IOBox('message');
            $msg->addMsg('Group ID: '.$form->attrs['id'].' was <b>updated</b>');
            $msg->addClose('OK');
            $msg->show();
        } catch(DatabaseQueryException $e) {
            $msg = new IOBox('message');
            $msg->addMsg('Cannot save group.');
            $msg->addClose('OK');
            $msg->show();
        }
    }
} catch(FormNotFoundException $e) {
    $msg = new IOBox('groups');
    $msg->target = $_SERVER['PHP_SELF'];
    $msg->addLabel('Create Group');
    $msg->addMsg('<table id="flagtable" border="1px" cellspacing="0" width="440px">
  <tr>
    <td colspan="2"><b>Privileges</b></td>
  </tr>
  <tr>
    <td valign="top" width="200">
		<input type="checkbox" value="8" onclick="calcFlags()"> Can not be attacked<br/>
		<input type="checkbox" value="16" onclick="calcFlags()"> Can convince all monsters<br/>
		<input type="checkbox" value="32" onclick="calcFlags()"> Can summon all monsters<br/>
		<input type="checkbox" value="64" onclick="calcFlags()"> Can illusion all monsters<br/>
		<input type="checkbox" value="128" onclick="calcFlags()"> Can sense invisibility<br/>
		<input type="checkbox" value="256" onclick="calcFlags()"> Ignored by monsters<br/>
		<input type="checkbox" value="512" onclick="calcFlags()"> Do not gain infight<br/>
		<input type="checkbox" value="1024" onclick="calcFlags()"> Has unlimited mana<br/>
		<input type="checkbox" value="2048" onclick="calcFlags()"> Has unlimited soul<br/>
		<input type="checkbox" value="4096" onclick="calcFlags()"> Do no gain exhaustion<br/>
		<input type="checkbox" value="32768" onclick="calcFlags()"> Can always login<br/>
		<input type="checkbox" value="65536" onclick="calcFlags()"> Can broadcast<br/>
		<input type="checkbox" value="131072" onclick="calcFlags()"> Can edit all house rights<br/>
		<input type="checkbox" value="262144" onclick="calcFlags()"> Can not be banned<br/>
    </td>
    <td valign="top" width="240">
		<input type="checkbox" value="524288" onclick="calcFlags()"> Can not be pushed<br/>
		<input type="checkbox" value="1048576" onclick="calcFlags()"> Has unlimited capacity<br/>
		<input type="checkbox" value="2097152" onclick="calcFlags()"> Can push all creatures<br/>
		<input type="checkbox" value="4194304" onclick="calcFlags()"> Talk red in private<br/>
		<input type="checkbox" value="8388608" onclick="calcFlags()"> Talk red in channel<br/>
		<input type="checkbox" value="16777216" onclick="calcFlags()"> Talk orange in help-channel<br/>
		<input type="checkbox" value="17179869184" onclick="calcFlags()"> Skip spell usage checks<br/>
		<input type="checkbox" value="34359738368" onclick="calcFlags()"> Skip weapon usage checks<br/>
		<input type="checkbox" value="536870912" onclick="calcFlags()"> Gain max speed<br/>
		<input type="checkbox" value="1073741824" onclick="calcFlags()"> Cannot be added to VIP<br/>
		<input type="checkbox" value="4294967296" onclick="calcFlags()"> Talk red anonymously<br/>
		<input type="checkbox" value="8589934592" onclick="calcFlags()"> Ignore protection-zone<br/>
		<input type="checkbox" value="68719476736" onclick="calcFlags()"> Can not be muted<br/>
		<input type="checkbox" value="137438953472" onclick="calcFlags()"> Is premium<br/>
		<input type="checkbox" value="274877906944" onclick="calcFlags()"> Can answer rule violations<br/>
		<input type="checkbox" value="549755813888" onclick="calcFlags()"> Can reload content<br/>
		<input type="checkbox" value="1099511627776" onclick="calcFlags()"> Show group instead vocation<br/>
        <input type="checkbox" value="2199023255552" onclick="calcFlags()"> Has infinite stamina<br/>
        <input type="checkbox" value="4398046511104" onclick="calcFlags()"> PlayerFlag_CannotMoveItems<br/>
        <input type="checkbox" value="8796093022208" onclick="calcFlags()"> PlayerFlag_CannotMoveCreatures<br/>
        <input type="checkbox" value="17592186044416" onclick="calcFlags()"> PlayerFlag_CanReportBugs<br/>
        <input type="checkbox" value="35184372088832" onclick="calcFlags()"> PlayerFlag_CanSeeSpecialDescription<br/>
        <input type="checkbox" value="70368744177664" onclick="calcFlags()"> PlayerFlag_CannotBeSeen<br/>
    </td></tr><tr>
    <td valign="top" width="200">
		<input type="checkbox" value="1" onclick="calcFlags()"> Can not use combat<br/>
		<input type="checkbox" value="2" onclick="calcFlags()"> Can not attack players<br/>
		<input type="checkbox" value="4" onclick="calcFlags()"> Can not attack monsters<br/>
		<input type="checkbox" value="8192" onclick="calcFlags()"> Cannot use spells<br/>
		<input type="checkbox" value="16384" onclick="calcFlags()"> Cannot pickup items<br/>
		<input type="checkbox" value="33554432" onclick="calcFlags()"> Do not gain experience<br/>
		<input type="checkbox" value="67108864" onclick="calcFlags()"> Do not gain mana<br/>
		<input type="checkbox" value="134217728" onclick="calcFlags()"> Do not gain health<br/>
		<input type="checkbox" value="268435456" onclick="calcFlags()"> Do not gain skill<br/>
		<input type="checkbox" value="2147483648" onclick="calcFlags()"> Can not gain loot<br/>
    </td><td>');
    $msg->addInput('id','text','0');
    $msg->addInput('name','text');
    $msg->addInput('flags','text','0');
    $msg->addInput('access','text','0');
    $msg->addInput('depot size','text','1000');
    $msg->addInput('vip size','text','100');
    $msg->addClose('Cancel');
    $msg->addSubmit('Save');
    $msg->addMsg('</td></tr></table>');
    $msg->show();
}

?>
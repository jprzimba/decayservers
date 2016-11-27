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

/*
 * REQUIRED POST DATA
 * guild_id
 * rank_id
 * rank_name
 */
$responseXML = new SimpleXMLElement('<response/>');

try {
//load account if loged in
    $account = new Account();
    $account->load($_SESSION['account']);

    //load guild
    $guild = new Guild();
    $guild->load($_POST['guild_id']);

    if ($guild->attrs['owner_acc'] != $account->attrs['accno'])
        throw new ModuleException('Permission denied.');

    if (!AAC::ValidGuildRank($_POST['rank_name']))
        throw new ModuleException('Not a valid rank name.');

    if (isset($_POST['rank_id'])) {
    //rename rank
        $guild->setRank($_POST['rank_id'], $_POST['rank_name']);
        $responseXML->addChild('error', 0);
        $responseXML->addChild('name', $guild->ranks[$_POST['rank_id']]['name']);
    } else {
    //create rank
        $rid = $guild->addRank($_POST['rank_name']);
        $responseXML->addChild('error', 0);
        $responseXML->addChild('name', $guild->ranks[$rid]['name']);
    }

} catch(ModuleException $e) {
    $responseXML->addChild('error', 1);
    $responseXML->addChild('message', $e->getMessage());

} catch (AccountException $e) {
    $responseXML->addChild('error', 1);
    $responseXML->addChild('message', 'There was a problem loading your account. Try to login again.');
}

echo $responseXML->asXML();
?>
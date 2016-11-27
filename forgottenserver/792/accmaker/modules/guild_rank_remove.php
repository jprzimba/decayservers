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

    $guild->removeRank($_POST['rank_id']);
    $responseXML->addChild('error', 0);

} catch(ModuleException $e) {
    $responseXML->addChild('error', 1);
    $responseXML->addChild('message', $e->getMessage());

} catch(RankIsUsedException $e) {
    $responseXML->addChild('error', 1);
    $responseXML->addChild('message', 'This rank is used. Please remove all players from it and try again.');
}

echo $responseXML->asXML();
?>
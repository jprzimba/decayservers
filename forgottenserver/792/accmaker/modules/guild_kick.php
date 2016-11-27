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
//load account if loged in
    $account = new Account();
    $account->load($_SESSION['account']);

    //load guild
    $guild = new Guild();
    $guild->load($_POST['guild_id']);

    //load the player
    $player = new Player();
    $player->find($_POST['player_name']);

    //check if user has rights to kick
    if (! ($guild->canKick($account->attrs['accno']) || $account->hasPlayer($player->attrs['id'])) )
        throw new ModuleException('You do not have permission.');
        
    $guild->playerLeave($player);
    $responseXML->addChild('error', 0);
    $responseXML->addChild('player', $player->attrs['name']);

} catch(ModuleException $e) {
    $responseXML->addChild('error', 1);
    $responseXML->addChild('message', $e->getMessage());

} catch(AccountNotFoundException $e) {
    $responseXML->addChild('error', 1);
    $responseXML->addChild('message', 'There was a problem loading your account. Try to login again.');

} catch(PlayerNotFoundException $e) {
    $responseXML->addChild('error', 1);
    $responseXML->addChild('message', 'Player not found.');
}

echo $responseXML->asXML();
?>
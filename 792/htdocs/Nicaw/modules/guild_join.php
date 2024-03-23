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

//load account if loged in
$account = new Account();
if (isset($_SESSION['account']) && $account->load($_SESSION['account'])) {
//load guild
    $guild = new Guild();
    if (isset($_REQUEST['guild_id']) && $guild->load($_REQUEST['guild_id'])) {
        //load the player
        $player = new Player();
        if ($player->find($_REQUEST['player_name'])) {
        //check if player belong to current account
            if ($player->attrs['account'] == $_SESSION['account']) {
                if ($guild->isInvited($player->attrs['id'])) {
                    //cant join if player belongs to another guild
                    if ($player->attrs['rank_id'] == 0) {
						if (!$player->isOnline()) {
							if ($guild->playerJoin($player)) {
								//success
							}else $error = 'Cannot join guild';
						}else $error = 'Cannot complete action. Player is online.';
                    }else $error = 'Cannot join because you are a member of another guild';
                }else $error = 'You are not invited';
            }else $error = 'Player does not belong to your account';
        }else $error = 'Cannot find this player';
    }else $error = 'Cannot load guild';
}else $error = 'You are not logged in';

$responseXML = new SimpleXMLElement('<response/>');
if (empty($error)) {
    $responseXML->addChild('error', 0);
    $responseXML->addChild('player', $player->attrs['name']);
} else {
    $responseXML->addChild('error', 1);
    $responseXML->addChild('message', $error);
}
echo $responseXML->asXML();
?>
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

//load account if loged in
$account = new Account();
if (isset($_SESSION['account']) && $account->load($_SESSION['account'])) {
//load guild
    $guild = new Guild();
    if (isset($_POST['guild_id']) && $guild->load($_POST['guild_id'])) {
        if ($guild->attrs['owner_acc'] == $account->attrs['accno']) {
            $_POST['rank_name'] = ucfirst($_POST['rank_name']);
            if (AAC::ValidGuildRank($_POST['rank_name'])) {
                if (isset($_POST['rank_id'])) {
                //rename rank
                    if ($guild->isRank($_POST['rank_id'])) {
                        if ($guild->setRank($_POST['rank_id'], $_POST['rank_name'])) {
                            $rank_id = $_POST['rank_id'];
                        }else $error = 'Renaming failed';
                    }else $error = 'Rank does not exist';
                } else {
                //create rank
                    $rank_id = $guild->addRank($_POST['rank_name']);
                    if ($rank_id) {
                        //success
                    }else $error = 'Cannot add rank';
                }
            }else $error = 'Not a valid rank name';
        }else $error = 'You do not have permission';
    }else $error = 'Cannot load guild';
}else $error = 'You are not logged in';

$responseXML = new SimpleXMLElement('<response/>');
if (empty($error)) {
    $responseXML->addChild('error', 0);
    $responseXML->addChild('name', $guild->ranks[$rank_id]['name']);
} else {
    $responseXML->addChild('error', 1);
    $responseXML->addChild('message', $error);
}
echo $responseXML->asXML();
?>
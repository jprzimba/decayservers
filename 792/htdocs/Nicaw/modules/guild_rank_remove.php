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

//load account if loged in
$account = new Account();
if (isset($_SESSION['account']) && $account->load($_SESSION['account'])) {
//load guild
    $guild = new Guild();
    if (isset($_POST['guild_id']) && $guild->load($_POST['guild_id'])) {
        if ($guild->attrs['owner_acc'] == $account->attrs['accno']) {
            if ($guild->isRank($_POST['rank_id'])) {
                if ($guild->removeRank($_POST['rank_id'])) {
                    //success
                }else $error = 'Cannot delete. Make sure no player is assigned to this rank.';
            }else $error = 'Rank not found';
        }else $error = 'You do not have permission';
    }else $error = 'Cannot load guild';
}else $error = 'You are not logged in';

$responseXML = new SimpleXMLElement('<response/>');
if (empty($error)) {
    $responseXML->addChild('error', 0);
} else {
    $responseXML->addChild('error', 1);
    $responseXML->addChild('message', $error);
}
echo $responseXML->asXML();
?>
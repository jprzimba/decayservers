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

try {
//retrieve post data
    $form = new Form('character');

    //load account if loged in
    $account = new Account();
    $account->load($_SESSION['account']);

    //create new player object
    $form->attrs['name'] = ucfirst($form->attrs['name']);
    $newplayer = new Player();

    //check for correct parameters
    if (!($cfg['temple'][$form->attrs['residence']]['enabled'] && $cfg['vocations'][(int)$form->attrs['vocation']]['enabled'] && preg_match("/^[01]$/",$form->attrs['sex'])))
        throw new ModuleException('Invalid parameters.');

    //check character number
    if (count($account->players) >= $cfg['maxchars'])
        throw new ModuleException('Maximum number of characters reached for your account.');

    //check for valid name
    if (!AAC::ValidPlayerName($form->attrs['name']))
        throw new ModuleException('<b>Not a valid name:</b><br/><ul><li>First letter capital</li><li>At least 4 characters, at most 25</li><li>No capital letters in midlle of word</li><li>Letters A-Z, -\' and spaces</li><li>Monster names not allowed</li></ul>');

    //player name must not exist
    if (Player::exists($form->attrs['name']))
        throw new ModuleException('A character with this name already exist.');

    //create character and add it to account
    $newplayer = Player::Create($form->attrs['name'], $_SESSION['account'], (int)$form->attrs['vocation'], (int)$form->attrs['sex'], (int)$form->attrs['residence']);
    $account->logAction('Created character: '.$newplayer->attrs['name']);

    //create new message
    $msg = new IOBox('message');
    $msg->addMsg('Your character was successfuly created.');
    $msg->addRefresh('Finish');
    $msg->show();


} catch(FormNotFoundException $e) {
//make a list of valid vocations
    while ($vocation = current($cfg['vocations'])) {
        if (isset($vocation['enabled']) && $vocation['enabled'])
            $vocations[key($cfg['vocations'])] = $vocation['name'];
        next($cfg['vocations']);
    }
    //make a list of valid spawn places
    while ($spawn = current($cfg['temple'])) {
        if (isset($spawn['enabled']) && $spawn['enabled']) {
            $spawns[key($cfg['temple'])] = $spawn['name'];
        }
        next($cfg['temple']);
    }
    //create new form
    $form = new IOBox('character');
    $form->target = $_SERVER['PHP_SELF'];
    $form->addLabel('Create Character');
    $form->addInput('name');
    $form->addSelect('sex',array(1 => 'Male', 0 => 'Female'));
    $form->addSelect('vocation',$vocations);
    $form->addSelect('residence',$spawns);
    $form->addClose('Cancel');
    $form->addSubmit('Next >>');
    $form->show();

} catch(ModuleException $e) {
    $msg = new IOBox('message');
    $msg->addMsg($e->getMessage());
    $msg->addReload('<< Back');
    $msg->addClose('OK');
    $msg->show();

} catch (AccountNotFoundException $e)  {
    $msg = new IOBox('message');
    $msg->addMsg('There was a problem loading your account. Try to login again.');
    $msg->addRefresh('OK');
    $msg->show();

}
?>
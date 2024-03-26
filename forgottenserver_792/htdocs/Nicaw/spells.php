<?php 
/*
    Copyright (C) 2006  Nicaw

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
include('config.inc.php');
include('include.inc.php');
$ptitle = "Server Info - $cfg[server_name]";
include("header.inc.php");
?>
<div id="content">
<div class="top">Spells</div>
<div class="mid">
<form method="get" action="spells.php">
<input type="text" name="spell"/> 
<input type="submit" value="Search"/> 
</form>

<?php
$SpellsXML = simplexml_load_file($cfg['dirdata'] . 'spells/spells.xml');
if ($SpellsXML === false) {
    throw new aacException('spells.xml not found');
}

foreach ($SpellsXML->instant as $spell) {
    $spellName = (string) $spell['name'];
    $spellWords = (string) $spell['words'];
    $spellLvl = (int) $spell['lvl'];
    $spellMagLvl = (int) $spell['maglv'];
    $spellMana = (int) $spell['mana'];
    $spellPrem = (int) $spell['prem'];
    $vocations = array();

    foreach ($spell->vocation as $vocation) {
        $vocations[] = (string) $vocation['name'];
    }

    if (empty($_GET['spell']) || (stripos($spellName, $_GET['spell']) !== false) || (stripos($spellWords, $_GET['spell']) !== false)) {
        echo '<table>';
        echo '<tr><td width="150px"><b>Spell Name</b></td><td>' . htmlspecialchars($spellName) . '</td></tr>';
        echo '<tr><td><b>Type</b></td><td>Instant</td></tr>';
        echo '<tr><td><b>Words</b></td><td>' . htmlspecialchars($spellWords) . '</td></tr>';
        if ($spellLvl > 0) {
            echo '<tr><td><b>Required Level</b></td><td>' . $spellLvl . '</td></tr>';
        }
        if ($spellMagLvl > 0) {
            echo '<tr><td><b>Magic Level</b></td><td>' . $spellMagLvl . '</td></tr>';
        }
        echo '<tr><td><b>Mana</b></td><td>' . $spellMana . '</td></tr>';
        echo '<tr><td><b>Premium</b></td><td>' . (($spellPrem == 1) ? 'yes' : 'no') . '</td></tr>';
        echo '<tr><td><b>Vocations</b></td><td>' . implode(', ', $vocations) . '</td></tr>';
        echo '</table><hr/>';
    }
}

foreach ($SpellsXML->conjure as $spell) {
    $spellName = (string) $spell['name'];
    $spellWords = (string) $spell['words'];
    $spellLvl = (int) $spell['lvl'];
    $spellMagLvl = (int) $spell['maglv'];
    $spellMana = (int) $spell['mana'];
    $spellPrem = (int) $spell['prem'];
    $spellSoul = (int) $spell['soul'];
    $spellConjureCount = (int) $spell['conjureCount'];
    $vocations = array();

    foreach ($spell->vocation as $vocation) {
        $vocations[] = (string) $vocation['name'];
    }

    if (empty($_GET['spell']) || (stripos($spellName, $_GET['spell']) !== false) || (stripos($spellWords, $_GET['spell']) !== false)) {
        echo '<table>';
        echo '<tr><td width="150px"><b>Spell Name</b></td><td>' . htmlspecialchars($spellName) . '</td></tr>';
        echo '<tr><td><b>Type</b></td><td>Conjure</td></tr>';
        echo '<tr><td><b>Words</b></td><td>' . htmlspecialchars($spellWords) . '</td></tr>';
        if ($spellLvl > 0) {
            echo '<tr><td><b>Required Level</b></td><td>' . $spellLvl . '</td></tr>';
        }
        if ($spellMagLvl > 0) {
            echo '<tr><td><b>Magic Level</b></td><td>' . $spellMagLvl . '</td></tr>';
        }
        echo '<tr><td><b>Mana</b></td><td>' . $spellMana . '</td></tr>';
        echo '<tr><td><b>Premium</b></td><td>' . (($spellPrem == 1) ? 'yes' : 'no') . '</td></tr>';
        echo '<tr><td><b>Vocations</b></td><td>' . implode(', ', $vocations) . '</td></tr>';
        echo '<tr><td><b>Soul Points</b></td><td>' . $spellSoul . '</td></tr>';
        echo '<tr><td><b>Conjure Count</b></td><td>' . $spellConjureCount . '</td></tr>';
        echo '</table><hr/>';
    }
}
?>

</div>
<div class="bot"></div>
</div>
<?include ('footer.inc.php');?>

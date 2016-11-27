<?php
/*
    Copyright (C) 2007-2008  Nicaw

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
include ("include.inc.php");
$ptitle="Online Players - $cfg[server_name]";
include ("header.inc.php");
?>
<div id="content">
    <div class="top">Online Players</div>
    <div class="mid">
        <?php
        $SQL = AAC::$SQL;
        try {
            $SQL->myQuery('SELECT name, vocation, level FROM players WHERE online = 1 ORDER BY name ASC');
        } catch(DatabaseQueryException $e) {
            try {
                $SQL->myQuery('SELECT name, vocation, level FROM players WHERE lastlogin > lastlogout ORDER BY name ASC');
            } catch(DatabaseQueryException $e) {
                throw new DatabaseException('Your server does not store information on players online state.');
            }
        }

        if ($SQL->num_rows() == 0) {
            echo 'Nobody is online :-O';
        } else {

            $i = 0;
            echo '<table><tr class="color0"><td style="width:150px"><b>Name</b></td><td style="width:150px"><b>Vocation</b></td><td style="width:60px"><b>Level</b></td></tr>';
            while ($player = $SQL->fetch_array()) {
                $i++;
                echo '<tr '.getStyle($i).'><td><a href="characters.php?player_name='.urlencode($player['name']).'">'.htmlspecialchars($player['name']).'</a></td><td>'.htmlspecialchars($cfg['vocations'][$player['vocation']]['name']).'</td><td>'.$player['level'].'</td></tr>'."\n";
            }
        }
        ?>
        </table>
    </div>
    <div class="bot"></div>
</div>
<?php include ("footer.inc.php");?>
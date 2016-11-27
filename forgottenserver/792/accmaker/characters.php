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
include ("include.inc.php");
$ptitle="Characters - $cfg[server_name]";
include("header.inc.php");
?>
<div id="content">
    <div class="top">Character Lookup</div>
    <div class="mid">
        <form method="get" action="characters.php">
            <input type="text" name="player_name"/>
            <input type="submit" value="Search"/>
        </form>
        <?php
        try {
            $player = new Player();
            if (!empty($_GET['player_id'])) {
                $player->load($_GET['player_id']);
            } elseif (!empty($_GET['player_name'])) {
                $player->find($_GET['player_name']);
            } else {
                throw new PlayerException();
            }
            $account = new Account();
            $account->load($player->attrs['account']);

            echo '<hr/><table style="width: 100%"><tr><td><b>Name:</b> '.htmlspecialchars($player->attrs['name']).'&nbsp;';
            if($player->isOnline()) echo '<span style="color:green">[Online]</span>'."<br/>\n";
            else echo '<span style="color:red">[Offline]</span>'."<br/>\n";
            echo '<b>Level:</b> '.$player->attrs['level']."<br/>\n";
            echo '<b>Magic Level:</b> '.$player->attrs['maglevel']."<br/>\n";
            echo '<b>Vocation:</b> '.$cfg['vocations'][$player->attrs['vocation']]['name']."<br/>\n";

            try {
                echo '<b>Guild:</b> '.$player->guild['guild_rank_name'].' of <a href="guilds.php?guild_id='.$player->guild['guild_id'].'">'.htmlspecialchars($player->guild['guild_name']).'</a><br/>'."\n";
            } catch(GuildNotFoundException $e) {}

            $gender = Array('Female','Male');
            echo '<b>Gender:</b> '.$gender[$player->attrs['sex']].'<br/>'."\n";
            if (!empty($cfg['temple'][$player->attrs['city']]['name']))
                echo "<b>Residence</b>: ".ucfirst($cfg['temple'][$player->attrs['city']]['name'])."<br/>";

            if (isset($player->attrs['position'])) {
                echo "<b>Position: </b> ".$player->attrs['position']."<br/>";
            }
            if ($account->attrs['premend'] > time()) {
                echo "<b>Premium: </b> ".ceil(($account->attrs['premend'] - time())/(3600*24))." day(s)<br/>";
            }
            if ($player->attrs['lastlogin'] == 0)
                $lastlogin = 'Never';
            else
                $lastlogin = date("jS F Y H:i:s",$player->attrs['lastlogin']);
            echo "<b>Last Login:</b> ".$lastlogin."<br/>\n";
            if ($cfg['show_skills']) {
                echo "</td><td>";
                $sn = $cfg['skill_names'];
                for ($i=0; $i < count($sn); $i++) {
                    echo '<b>'.ucfirst($sn[$i]).':</b> '.$player->skills[$i]['skill']."<br/>\n";
                }
                echo '</td></tr>';
            }
            echo '</table>';
            if (strlen($account->attrs['comment'])>0) {
                echo "<b>Comments</b><br/><div style=\"overflow:hidden\"><pre>".htmlspecialchars($account->attrs['comment'])."</pre></div><br/>\n";
            }
            echo '<hr/>';
            if ($account->attrs['reveal_characters'] && $account->players && count($account->players) > 1) {
                echo '<b>Characters on the same account</b><br/><ul class="task-menu">';
                foreach ($account->players as $_player) {
                    echo '<li style="background-image: url(resource/user.png);" onclick="window.location.href=\'characters.php?player_id='.htmlspecialchars($_player['id']).'\'">'.htmlspecialchars($_player['name']).'</li>';
                }
                echo '</ul><hr/>';
            }

            if ($cfg['show_deathlist']) {
                if ($player->deaths) {
                    echo '<b>Latest Deaths</b><br/>';
                    $prevdate = 0;
                    foreach ($player->deaths as $death) {
                        if ($death['killer_id'])
                            $name = '<a href="characters.php?player_id='.$death['killer_id'].'">'.$death['killer_name'].'</a>';
                        else
                            $name = $death['killer_name'];
                        if($prevdate == $death['date'])
                            echo '&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;and by '.$name.'<br/>';
                        else
                            echo '<i>'.date("jS F Y H:i:s",$death['date']).'</i> Killed at level '.$death['victim_level'].' by '.$name.'<br/>';
                        $prevdate = $death['date'];
                    }
                }
            }
        } catch(PlayerNotFoundException $e) {
            echo 'Player not found';
        } catch(PlayerException $e) {}
        ?>
    </div>
    <div class="bot"></div>
</div>
<?php include ("footer.inc.php");?>
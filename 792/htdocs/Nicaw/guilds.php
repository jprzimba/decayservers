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
$ptitle="Guilds - $cfg[server_name]";
include ("header.inc.php");
$SQL = AAC::$SQL;
?>
<div id="content">
    <div class="top">Guilds</div>
    <div class="mid">
        <form method="get" action="guilds.php">
            <input type="text" name="guild_name"/>
            <input type="submit" value="Search"/>
        </form>
        <hr style="margin-top: 5px; margin-bottom: 5px; "/>
        <?php
        //-----------------------Guild list
        if (!isset($_GET['guild_id']) && !isset($_GET['guild_name'])) {
            $query = 'SELECT guilds.id, guilds.name, nicaw_guild_info.description FROM guilds LEFT JOIN nicaw_guild_info ON guilds.id = nicaw_guild_info.id ORDER BY name ASC';
            $SQL->myQuery($query);
            if ($SQL->failed())
                throw new aacException('SQL query failed:<br/>'.$SQL->getError());
            while ($a = $SQL->fetch_array()) {
                if (file_exists('guilds/'.$a['id'].'.gif'))
                    $img_path = 'guilds/'.$a['id'].'.gif';
                else
                    $img_path = 'resource/guild_default.gif';
                ?>
        <table border="1" onclick="window.location.href='guilds.php?guild_id=<?php echo urlencode($a['id'])?>'" style="cursor: pointer; width: 100%;">
            <tr><td style="width: 64px; height: 64px; padding: 10px;"><img src="<?php echo $img_path?>" alt="NO IMG" height="64" width="64"/></td>
                <td style="vertical-align: top;">
                    <b><?php echo htmlspecialchars($a['name'])?></b><hr/>
                            <?php echo htmlspecialchars($a['description'])?>
                </td></tr>
        </table>

            <?php }
        }else {
        //-------------------------Member list
            $guild = new Guild();
            if (!empty($_GET['guild_id']) && !$guild->load($_GET['guild_id']))
                echo 'Guild not found.';
            elseif (!empty($_GET['guild_name']) && !$guild->find($_GET['guild_name']))
                echo 'Guild not found.';
            else {
                if (file_exists('guilds/'.$guild->attrs['id'].'.gif'))
                    $img_path = 'guilds/'.$guild->attrs['id'].'.gif';
                else
                    $img_path = 'resource/guild_default.gif';
                ?>
        <table style="width: 100%"><tr><td style="width: 64px; height: 64px; padding: 10px;"><img src="<?php echo $img_path?>" alt="Guild IMG" height="64" width="64"/></td><td style="text-align: center">
                    <h1 style="display: inline"><?php echo htmlspecialchars($guild->attrs['name'])?>
                    </h1></td><td style="width: 64px; height: 64px; padding: 10px;">
                    <img src="<?php echo $img_path?>" alt="Guild IMG" height="64" width="64"/></td></tr>
        </table>
        <p><?php echo htmlspecialchars($guild->attrs['description'])?></p><hr/>
        <ul class="task-menu" style="width: 200px;">
            <li style="background-image: url(resource/book_previous.png);" onclick="self.window.location.href='guilds.php'">Back</li>
                    <?php
                    $is_owner = false;
                    if (!empty($_SESSION['account'])) {
                        $account = new Account();
                        if (!$account->load($_SESSION['account'])) die('Cannot load account');
                        $is_owner = $guild->attrs['owner_acc'] == $account->attrs['accno'];
                        $invited = false;
                        $member = false;
                        foreach ($account->players as $player) {
                            if ($guild->isInvited($player['id']))
                                $invited = true;
                            if ($guild->isMember($player['id']))
                                $member = true;
                        }
                        if ($is_owner) {?>
            <li style="background-image: url(resource/user_edit.png);" onclick="ajax('form','modules/guild_edit.php','guild_id=<?php echo $guild->attrs['id']?>',true)">Promote / Demote</li>
            <li style="background-image: url(resource/image_add.png);" onclick="ajax('form','modules/guild_image.php','guild_id=<?php echo $guild->attrs['id']?>',true)">Upload Image</li>
            <li style="background-image: url(resource/page_edit.png);" onclick="ajax('form','modules/guild_comments.php','guild_id=<?php echo $guild->attrs['id']?>',true)">Edit Description</li>
                    <?php 	}?>
            <li style="background-image: url(resource/resultset_previous.png);" onclick="self.window.location.href='login.php?logout&amp;redirect=account.php'">Logout</li>
                    <?php }else {?>
            <li style="background-image: url(resource/resultset_next.png);" onclick="self.window.location.href='login.php?redirect=guilds.php'">Login</li>
                    <?php }?>
        </ul><hr/>
        <h2 style="display: inline">Guild Members</h2>
        <table style="width: 100%">
                    <?php
                    echo '<tr class="color0">';
                    echo '<td style="width: 130px"><b>Rank</b></td>';
                    echo '<td style="width: 130px"><b>Name</b></td>';
                    echo '<td style="width: 150px"><b>Title</b></td>';
                    echo '</tr>';

                    $i = 0;
                    foreach ($guild->ranks as $rank) {
                        $i++;
                        if($is_owner) {
                            $rank_content = '<td id="rank'.$rank['id'].'"><img style="cursor: pointer" src="resource/page_edit.png" alt="edit" height="16" width="16" onclick="Guild.prepareRankRename('.$guild->attrs['id'].', '.$rank['id'].',\''.htmlspecialchars($rank['name']).'\')"/>'.
                            '&nbsp;<img style="cursor: pointer" src="resource/cross.png" alt="del" height="16" width="16" onclick="Guild.requestRankDelete('.$guild->attrs['id'].', '.$rank['id'].')"/>&nbsp;'.
                            htmlspecialchars($rank['name']).'</td>';
                        } else {
                            $rank_content = '<td id="rank'.$rank['id'].'">'.htmlspecialchars($rank['name']).'</td>';
                        }
                        $rank_has_players = false;
                        foreach ($guild->members as $player) {
                            if ($player['rank'] != $rank['id']) continue;
                            $rank_has_players = true;
                            
                            if($is_owner) {
                                $title_content = htmlspecialchars($player['nick']).
                                    '&nbsp;<img style="cursor: pointer" src="resource/page_edit.png" alt="edit" height="16" width="16" onclick="Guild.prepareNickChange('.$guild->attrs['id'].', '.$player['id'].',\''.htmlspecialchars($player['nick']).'\')"/>';
                            } else {
                                $title_content = htmlspecialchars($player['nick']);
                            }
                            
                            if(isset($account) && ($guild->canKick($account->attrs['accno']) || $account->hasPlayer($player['id']))) {
                                $player_content = '<img style="cursor: pointer" src="resource/cross.png" alt="X" height="16" width="16" onclick="Guild.requestKick(\'player'.$player['id'].'\', \''.$player['name'].'\', '.$guild->attrs['id'].')"/>&nbsp;'.
                                '<a href="characters.php?player_id='.$player['id'].'">'.htmlspecialchars($player['name']).'</a>';
                            } else {
                                $player_content = '<a href="characters.php?player_id='.$player['id'].'">'.htmlspecialchars($player['name']).'</a>';
                            }
                            
                            echo '<tr '.getStyle($i).' id="player'.$player['id'].'">'
                                .$rank_content.'<td>'.$player_content.'</td><td id="player'.$player['id'].'_title">'
                                .$title_content.'</td></tr>';

                            $rank_content = '<td></td>';
                        }
                        //owner wants to see all ranks
                        if (!$rank_has_players && $is_owner) {
                            echo '<tr '.getStyle($i).'>'
                                .$rank_content.'<td></td><td></td></tr>';
                        }
                    }

                    if($is_owner) {
                        echo '<tr><td colspan="4"><input type="text" id="new_rank_name" value="rank name" style="font-style: italic" onclick="input_clear(this)"/>&nbsp;<img style="cursor: pointer" src="resource/add.png" alt="+" id="rank_button" onclick="Guild.requestAddRank('.$guild->attrs['id'].', $(\'new_rank_name\').value)" /></td></tr>';
                    }

                    echo '</table><h2 style="display: inline">Invited Players</h2>';
                    echo '<table id="table_invited" style="width: 100%">';
                    echo '<tr class="color0"><td colspan="2"><b>Name</b></td></tr>';

                    $i = 0;
                    foreach ($guild->invited as $a) {
                        echo '<tr '.getStyle($i++).' id="player'.$a['id'].'"><td>'.$a['name'].'</td><td>';
                        if(isset($account) && ($guild->canKick($account->attrs['accno']) || $account->hasPlayer($a['id']))) {
                            echo '<img style="cursor: pointer" src="resource/cross.png" alt="X" height="16" width="16" onclick="Guild.requestKick(\'player'.$a['id'].'\', \''.$a['name'].'\', '.$guild->attrs['id'].')"/>';
                        }
                        if(isset($account) && $account->hasPlayer($a['id'])) {
                            echo '<img style="cursor: pointer" src="resource/accept.png" alt="V" height="16" width="16" onclick="Guild.requestJoin(\''.$a['name'].'\', '.$guild->attrs['id'].')"/>';
                        }
                        echo '</td></tr>';
                    }
                    if(isset($account) && $guild->canInvite($account->attrs['accno'])) {
                        echo '<tr><td colspan="2"><input type="text" id="invite_name" value="player name" style="font-style: italic" onclick="input_clear(this)"/>&nbsp;<img style="cursor: pointer" src="resource/add.png" alt="+" id="invite_button" onclick="Guild.requestInvite($(\'invite_name\').value, '.$guild->attrs['id'].')" /></td></tr>';
                    }
                    echo '</table>';

            }//guild loading
            }//display guild
            ?>
    </div>
    <div class="bot"></div>
</div>
<?php include('footer.inc.php');?>
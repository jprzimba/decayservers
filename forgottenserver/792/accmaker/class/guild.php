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

class Guild {
    private $attrs, $sql;
    private $members = array();
    private $invited = array();
    private $ranks = array();

    public function __construct() {
        $this->sql = AAC::$SQL;
    }

    public function find($name) {
        if (empty($name)) throw new GuildNotFoundException();
        $guild = $this->sql->myRetrieve('guilds', array('name' => $name));
        if ($guild === false) throw new GuildNotFoundException();
        $this->load($guild['id']);
        return true;
    }

    private function load_members() {
        if(empty($this->attrs['id'])) throw new GuildNotLoadedException();
        $this->sql->myQuery('SELECT players.name, players.guildnick, players.id, guild_ranks.id AS rank FROM guild_ranks, players WHERE players.rank_id = guild_ranks.id AND guild_ranks.guild_id = '.$this->sql->quote($this->attrs['id']));

        while ($a = $this->sql->fetch_array()) {
            $this->members[$a['id']] = array('id' => $a['id'], 'name' => $a['name'], 'rank' => $a['rank'], 'nick' => $a['guildnick']);
        }

        return true;
    }

    private function load_invited() {
        if(empty($this->attrs['id'])) throw new GuildNotLoadedException();
        $this->sql->myQuery('SELECT players.id, players.name, nicaw_guild_invites.rank FROM players, guilds, nicaw_guild_invites WHERE players.id = nicaw_guild_invites.pid AND guilds.id = nicaw_guild_invites.gid AND guilds.id = '.$this->sql->quote($this->attrs['id']));

        while ($a = $this->sql->fetch_array())
            $this->invited[$a['id']] = array('id' => $a['id'], 'name' => $a['name'], 'rank' => $a['rank']);

        return true;
    }

    public function load($id) {
        if (!is_numeric($id)) throw new GuildNotFoundException();

        $query = 'SELECT players.account_id, guilds.* FROM players, guilds WHERE players.id = guilds.ownerid AND guilds.id = '.$this->sql->quote($id);
        $this->sql->myQuery($query);
        if ($this->sql->num_rows() > 1)
            throw new DatabaseException('Unexpected SQL answer. More than one guild exists with ID #'.$id.'.');
        if ($this->sql->num_rows() == 0)
            throw new GuildNotFoundException();
        $a = $this->sql->fetch_array();
        $this->attrs['id'] = (int) $a['id'];
        $this->attrs['name'] = (string) $a['name'];
        $this->attrs['owner_id'] = (int) $a['ownerid'];
        $this->attrs['owner_acc'] = (string) $a['account_id'];

        $this->sql->myQuery('SELECT * FROM nicaw_guild_info WHERE id = '.$this->sql->quote($this->attrs['id']));
        $a = $this->sql->fetch_array();
        $this->attrs['description'] = (string) $a['description'];

        $this->sql->myQuery('SELECT id, name, level FROM guild_ranks WHERE guild_id = '.$this->sql->quote($this->attrs['id']).' ORDER BY level DESC');
        while ($a = $this->sql->fetch_array()) {
            $this->ranks[$a['id']] = array('id' => $a['id'], 'name' => $a['name'], 'level' => $a['level']);
        }

        return true;
    }

    public function setDescription($text) {
        if(empty($this->attrs['id'])) throw new GuildNotLoadedException();

        $this->sql->myUpdate('nicaw_guild_info',
        array('description' => $text), array('id' => $this->attrs['id']));

        $this->attrs['description'] = $text;
        return true;
    }

    public function setOwner($player) {
        if(empty($this->attrs['id'])) throw new GuildNotLoadedException();

        $this->sql->myUpdate('guilds',
        array('ownerid' => $player->attrs['id']), array('id' => $this->attrs['id']));

        $this->attrs['owner_id'] = $player->attrs['id'];
        $this->attrs['owner_acc'] = $player->attrs['account'];
        return true;
    }

    public function setName($name) {
        if(empty($this->attrs['id'])) throw new GuildNotLoadedException();

        $this->sql->myUpdate('guilds',
        array('name' => $name), array('id' => $this->attrs['id']));

        $this->attrs['name'] = $name;
        return true;
    }

    public function __get($attr) {
        if(empty($this->attrs['id']))
            throw new ClassException('Attempt to get attribute of guild that is not loaded.');
        if($attr == 'attrs') {
            return $this->attrs;
        }elseif($attr == 'ranks') {
            return $this->ranks;
        }elseif($attr == 'members') {
            if(empty($this->members)) $this->load_members();
            return $this->members;
        }elseif($attr == 'invited') {
            if(empty($this->invited)) $this->load_invited();
            return $this->invited;
        }else {
            throw new ClassException('Undefined property: '.$attr);
        }
    }

    public function addRank($name, $level = null) {

        if ($level == null) {
            $level = 1;
        }

        $this->shift($level, true);

        $d['name'] = (string) $name;
        $d['level'] = (int) $level;
        $d['guild_id'] = $this->attrs['id'];

        $this->sql->myInsert('guild_ranks', $d);

        $this->ranks[$this->sql->insert_id()] = array('name' => $d['name'], 'level' => $d['level']);
        return $this->sql->insert_id();
    }

    public function getRankID($name) {
        foreach($this->ranks as $rank) {
            if(strcasecmp($rank['name'], $name) == 0) return $rank['id'];
        }
        return false;
    }

    public function isRankUsed($id) {
        foreach($this->__get('members') as $member) {
            if($member['rank'] == $id) return true;
        }
        return false;
    }

    public function removeRank($id) {
        if($this->isRankUsed($id)) throw new RankIsUsedException();
        if(!isset($this->ranks[$id])) throw new RankNotFoundException();
        $this->shift($this->ranks[$id]['level'], false);
        unset($this->ranks[$id]);
        $this->sql->myDelete('guild_ranks', array('id' => $id));
        return true;
    }

    public function setRank($id, $name, $level = null) {
        if(!isset($this->ranks[$id])) throw new RankNotFoundException();
        $d['name'] = (string) $name;
        if (isset($level)) {
            $d['level'] = (int) $level;
        }

        $this->sql->myUpdate('guild_ranks', $d, array('id' => $id));

        $this->ranks[$id] = array('name' => $name, 'level' => $level);
        return true;
    }

    public function isRank($id) {
        return array_key_exists($id, $this->ranks);
    }

    public function isInvited($id) {
        return array_key_exists($id, $this->__get('invited'));
    }

    public function isMember($id) {
        return array_key_exists($id, $this->__get('members'));
    }

    public function getMinMaxRankId($param) {
        if ($param) $str = 'MAX';
            else $str = 'MIN';
        $query =
        'SELECT id FROM guild_ranks '.
        'WHERE level = '.
        '(SELECT '.$str.'(level) FROM guild_ranks '.
        'WHERE guild_id = '.
        $this->sql->quote($this->attrs['id']).
        ') AND guild_id = '.
        $this->sql->quote($this->attrs['id']);

        $this->sql->myQuery($query);
        $a = $this->sql->fetch_array();
        return $a[0];
    }

    public function shift($lvl, $upDown) {
        $query = 'UPDATE guild_ranks SET level = level ';
        if ($upDown) {
            $query.= '+';
        } else {
            $query.= '-';
        }
        $query.= ' 1 WHERE guild_id = '.$this->sql->quote($this->attrs['id']).
        ' AND level >= '.$this->sql->quote($lvl);
        $this->sql->myQuery($query);
    }

    public function playerInvite($player, $rank_id = null) {
    //player is already a member - do nothing
        if ($this->isMember($player->attrs['id']) || $this->isInvited($player->attrs['id']))
            throw new MemberExistsException();

        if ($rank_id == null) {
            $rank_id = 0;
        }

        $this->sql->myInsert('nicaw_guild_invites', array('gid' => $this->attrs['id'], 'pid' => $player->attrs['id'], 'rank' => $rank_id));

        $this->invited[$player->attrs['id']] = array('name' => $player->attrs['name'], 'rank' => $rank_id);
        return true;
    }

    private function unInvite($pid) {
        $this->sql->myDelete('nicaw_guild_invites',
        array('gid' => $this->attrs['id'], 'pid' => $pid), 1);
        return true;
    }

    public function playerJoin($player, $rank_id = null) {
        //player is not already a member
        if ($this->isMember($player->attrs['id'])) 
            throw new MemberExistsException();

        if (!$this->isRank($rank_id))
            if (array_key_exists($player->attrs['id'], $this->invited))
                $rank_id = $this->invited[$player->attrs['id']]['rank'];
        if (!$this->isRank($rank_id))
            $rank_id = $this->getMinMaxRankId(false);
        if (!$this->isRank($rank_id))
            throw new RankNotFoundException();

        $player->setAttr('rank_id', $rank_id);

        $player->save_guild();

        if($this->isInvited($player->attrs['id'])) {
            $this->unInvite($player->attrs['id']);
            $this->members[$player->attrs['id']] = $this->invited[$player->attrs['id']];
            unset($this->invited[$player->attrs['id']]);
        } else {
            $this->members[$player->attrs['id']] = array('name' => $player->attrs['name'], 'rank' => $rank_id);
        }

        return true;
    }

    public function playerLeave($player) {
        if ($this->isMember($player->attrs['id'])) {
            $player->setAttr('rank_id', 0);
            $player->setAttr('guildnick', '');
            $player->save_guild();
            unset($this->members[$player->attrs['id']]);
            if (count($this->members) == 0) $this->remove();
            return true;
        } elseif ($this->isInvited($player->attrs['id'])) {
            $this->unInvite($player->attrs['id']);
            unset($this->invited[$player->attrs['id']]);
            return true;
        } else throw new PlayerNotFoundException();
    }
    
    public function canInvite($accno) {
        return $this->attrs['owner_acc'] == $accno;
    }
    
    public function canKick($accno) {
        return $this->attrs['owner_acc'] == $accno;
    }
    
    public static function exists($name) {
        $SQL = AAC::$SQL;
        $SQL->myQuery('SELECT * FROM `guilds` WHERE `name` = '.$SQL->quote($name));
        if ($SQL->num_rows() > 0) return true;
        return false;
    }
    
    public static function Create($guild_name, $owner_id) {
        $d['name'] = $guild_name;
        $d['ownerid'] = $owner_id;

        $SQL = AAC::$SQL;
        $SQL->myInsert('guilds', $d);

        $guild = new Guild();
        $guild->load($SQL->insert_id());

        $SQL->myReplace('nicaw_guild_info', array('description' => '', 'id' => $SQL->insert_id() ) );

        return $guild;
    }

    public function remove() {
        $this->sql->myQuery('UPDATE players SET rank_id = 0, guildnick = \'\' WHERE players.rank_id IN (SELECT id FROM guild_ranks WHERE guild_id = '.$this->sql->quote($this->attrs['id']).')');
        $this->sql->myQuery('DELETE FROM guilds WHERE id = '.$this->sql->quote($this->attrs['id']));
        $this->sql->myQuery('DELETE FROM nicaw_guild_info WHERE id = '.$this->sql->quote($this->attrs['id']));
        $this->sql->myQuery('DELETE FROM nicaw_guild_invites WHERE gid = '.$this->sql->quote($this->attrs['id']));
        $this->sql->myQuery('DELETE FROM guild_ranks WHERE guild_id = '.$this->sql->quote($this->attrs['id']));
        if (file_exists('guilds/'.$this->attrs['id'].'.gif')) {
            unlink('guilds/'.$this->attrs['id'].'.gif');
        }
        unset($this->attrs);
        return true;
    }
}
?>
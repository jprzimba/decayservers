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
class Account {
    private $attrs, $sql, $guilds;
    private $players = [];

    public function __construct() {
        $this->sql = AAC::$SQL;
    }

    public function find($accnumber) {
        $acc = $this->sql->myRetrieve('accounts', array('id' => $accnumber));
        if ($acc === false) return false;
        $this->load($acc['id']);
        return true;
    }

    public function load_guilds() {
        if(empty($this->attrs['accno'])) return false;
        $this->sql->myQuery('SELECT guilds.id, guilds.name FROM guilds, accounts, players WHERE players.id = guilds.ownerid AND players.account_id = accounts.id AND accounts.id = '.$this->sql->quote($this->attrs['accno']));
        if ($this->sql->failed()) return false;
        while ($a = $this->sql->fetch_array()) {
            $this->guilds[] = array('id' => $a['id'], 'name' => $a['name']);
        }
        return true;
    }

    public function load_players() {
        if(empty($this->attrs['accno'])) return false;
        $this->sql->myQuery('SELECT players.id, players.name FROM players WHERE (`account_id`='.$this->sql->quote($this->attrs['accno']).')');
        if ($this->sql->failed()) throw new aacException($this->sql->getError());
        while ($a = $this->sql->fetch_array()) {
            $this->players[] = array('name' => $a['name'], 'id' => $a['id']);
        }
        return true;
    }

    public function load($id) {
        if (!is_numeric($id)) return false;
        //load attributes from database
        $acc = $this->sql->myRetrieve('accounts', array('id' => $id));
        $nicaw_acc = $this->sql->myRetrieve('nicaw_accounts', array('account_id' => $id));
        if ($nicaw_acc === false) {
            return false;
        }

        if ($this->sql->failed()) throw new aacException('Cannot load account:<br/>'.$this->sql->getError());
        if ($acc === false) {
            return false;
        }
		if (isset($acc['blocked']) && $acc['blocked']) {
			return false;
		}
        //arranging attributes, ones on the left will be used all over the aac
        $this->attrs['accno'] = (int) $acc['id'];
        $this->attrs['password'] = (string) $acc['password'];
        $this->attrs['email'] = (string) $acc['email'];
        $this->attrs['rlname'] = $nicaw_acc['rlname'];
        $this->attrs['location'] = $nicaw_acc['location'];
        $this->attrs['comment'] = $nicaw_acc['comment'];
        $this->attrs['recovery_key'] = $nicaw_acc['recovery_key'];
        $this->attrs['reveal_characters'] = (bool) $nicaw_acc['reveal_characters'];
        if (isset($acc['premdays']) && $acc['premdays'] > 0) $this->attrs['premend'] = $acc['premdays']*24*3600 + time();
        elseif (isset($acc['premend']) && $acc['premend'] > 0) $this->attrs['premend'] = $acc['premend'];
        return true;
    }

    public function save() {
        $acc['id'] = $this->attrs['accno'];
        $acc['password'] = $this->attrs['password'];
        $acc['email'] = $this->attrs['email'];

        if (!$this->sql->myUpdate('accounts',$acc, array('id' => $this->attrs['accno'])))
            throw new aacException('Cannot save account:<br/>'.$this->sql->getError());

        $nicaw_acc['account_id'] = $this->attrs['accno'];
        $nicaw_acc['rlname'] = $this->attrs['rlname'];
        $nicaw_acc['location'] = $this->attrs['location'];
        $nicaw_acc['comment'] = $this->attrs['comment'];
        $nicaw_acc['recovery_key'] = $this->attrs['recovery_key'];
        $nicaw_acc['reveal_characters'] = $this->attrs['reveal_characters'];

        if (!$this->sql->myReplace('nicaw_accounts',$nicaw_acc))
            throw new aacException('Cannot save account:<br/>'.$this->sql->getError());

        return true;
    }

    public function hasPlayer($pid) {
        $players = $this->__get('players');
        foreach($players as $p) {
            if ($p['id'] == $pid) {
                return true;
            }
        }
        return false;
    }

    static public function Create($name, $password, $email, $rlname = '', $location = '') {
        $SQL = AAC::$SQL;

        unset($d);
        $d['password'] = Account::encodePassword($password);
        $d['email'] = $email;
        if (!$SQL->myInsert('accounts',$d)) throw new aacException('Account::Create() Cannot insert attributes:<br/>'.$SQL->getError());
        $aid = $SQL->insert_id();

        unset($d);
        $d['account_id'] = $aid;
        $d['rlname'] = $rlname;
        $d['location'] = $location;
        $SQL->myInsert('nicaw_accounts',$d);

        $account = new Account();
        if($account->load($aid))
            return $account;
        else
            return null;
    }

    public function __get($attr) {
        if(empty($this->attrs['accno']))
            throw new aacException('Attempt to get attribute of account that is not loaded.');
        if($attr == 'attrs') {
            return $this->attrs;
        }elseif($attr == 'guilds') {
            if(empty($this->guilds)) $this->load_guilds();
            return $this->guilds;
        }elseif($attr == 'players') {
            if(empty($this->players)) $this->load_players();
            return $this->players;
        }else {
            throw new aacException('Undefined property: '.$attr);
        }
    }

    public function setAttr($attr,$value) {
        $this->attrs[$attr] = $value;
    }

    public function setPassword($pass) {
        if(empty($pass)) return false;
        $this->attrs['password'] = Account::encodePassword($pass);
        return true;
    }

    static private function encodePassword($pass){global $cfg;
        if ($cfg['password_type'] == 'md5')
            $pass = md5($pass);
        elseif ($cfg['password_type'] == 'sha1')
            $pass = sha1($pass);
        return $pass;
    }

    public function checkPassword($pass) {global $cfg;
        if ($cfg['password_type'] == 'md5') {
            $pass = md5($pass);
        }elseif ($cfg['password_type'] == 'sha1') {
            $pass = sha1($pass);
        }elseif ($cfg['password_type'] == 'plain') {
        }else throw new aacException('Unknow password encoding.');
        return $this->attrs['password'] == (string)$pass && !empty($pass);
    }

    static public function accountNumberExists($number) {
        AAC::$SQL->myQuery('SELECT * FROM `accounts` WHERE `id` = '.AAC::$SQL->quote($number));
        if (AAC::$SQL->failed()) throw new aacException('Account::accountNumberExists() failed. If your server doesn\'t support account number pelase use AAC modified by Tryller');
        if (AAC::$SQL->num_rows() > 0) return true;
        return false;
    }

    public function logAction($action) {
        $this->sql->myQuery('INSERT INTO `nicaw_account_logs` (id, ip, account_id, date, action) VALUES(NULL, INET_ATON('.$this->sql->quote($_SERVER['REMOTE_ADDR']).'), '.$this->sql->quote($this->attrs['accno']).', UNIX_TIMESTAMP(NOW()), '.$this->sql->quote($action).')');
        if ($this->sql->failed())
            throw new aacException($this->sql->getError());
    }

    public function removeRecoveryKey() {
        $this->attrs['recovery_key'] = NULL;
    }

    public function addRecoveryKey() {
        $this->attrs['recovery_key'] = substr(str_shuffle(md5(mt_rand()).md5(time())), 0, 32);
        $this->logAction('Recovery key added');
        return $this->attrs['recovery_key'];
    }

    public function checkRecoveryKey($key) {
        return $this->attrs['recovery_key'] === $key && !empty($key);
    }

    public function vote($option) {
        $this->sql->myQuery('INSERT INTO `nicaw_poll_votes` (option_id, ip, account_id) VALUES('.$this->sql->quote($option).', INET_ATON('.$this->sql->quote($_SERVER['REMOTE_ADDR']).'), '.$this->sql->quote($this->attrs['accno']).')');
        if ($this->sql->failed())
            throw new aacException($this->sql->getError());
    }

    public function getMaxLevel() {
        $this->sql->myQuery('SELECT MAX(level) AS maxlevel FROM `players` WHERE `account_id` = '.$this->sql->quote($this->attrs['accno']));
        if ($this->sql->failed())
            throw new aacException($this->sql->getError);
        $row = $this->sql->fetch_array();
        return (int) $row['maxlevel'];
    }

    public function canVote($option) {
        $query = 'SELECT nicaw_polls.id FROM nicaw_polls, nicaw_poll_options
WHERE nicaw_polls.id = nicaw_poll_options.poll_id
AND nicaw_poll_options.id = '.$this->sql->quote($option).'
AND '.$this->sql->quote($this->getMaxLevel()).' > minlevel
AND nicaw_polls.startdate < UNIX_TIMESTAMP(NOW())
AND nicaw_polls.enddate > UNIX_TIMESTAMP(NOW())';
        $this->sql->myQuery($query);
        if ($this->sql->failed())
            throw new aacException($this->sql->getError);
        if ($this->sql->num_rows() == 0) return false;
        if ($this->sql->num_rows() > 1) throw new aacException('Unexpected SQL answer.');
        $a = $this->sql->fetch_array();
        $poll_id = $a['id'];
        $query = 'SELECT * FROM nicaw_poll_votes, nicaw_poll_options
WHERE nicaw_poll_options.poll_id = '.$this->sql->quote($poll_id).'
AND nicaw_poll_options.id = nicaw_poll_votes.option_id
AND (account_id = '.$this->sql->quote($this->attrs['accno']).' OR ip = INET_ATON('.$this->sql->quote($_SERVER['REMOTE_ADDR']).')
)';
        $this->sql->myQuery($query);
        if ($this->sql->failed())
            throw new aacException($this->sql->getError);
        if ($this->sql->num_rows() == 0) return true;
        elseif ($this->sql->num_rows() == 1) return false;
        else throw new aacException('Unexpected SQL answer.');
    }
}
?>
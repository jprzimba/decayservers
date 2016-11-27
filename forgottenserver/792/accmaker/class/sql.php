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
class SQL {
    private
    $sql_connection,
    $schema_version,
    $sql_tables,
    $last_query,
    $last_insert_id;

    //creates new connection
    public function __construct($server, $user, $password, $database) {

        //warn if MySQL extension is not installed
        if(!extension_loaded('mysql'))
            throw new LibraryMissingException('MySQL library is not installed. Database access is impossible.', 0);

        //establish a link to MySQL
        $con = @mysql_connect($server,$user,$password);
        if ($con === false)
            throw new DatabaseConnectException('Unable to connect to mysql server. Please make sure it is up and running and you have correct user/password in config.inc.php.', 1);

        //select otserv database
        if (!mysql_select_db($database))
            throw new DatabaseSelectException('Unable to select database: '.$database.'. Make sure it exists.', 2);

        //retrieve table list
        $result = mysql_query('SHOW TABLES');
        if ($result === false)
            DatabaseQueryException('Failed to retrieve a table list.');

        while ($a = mysql_fetch_array($result))
            $this->sql_tables[] = $a[0];

        //retrieve schema version
        $result = mysql_query('SELECT value FROM schema_info WHERE name = \'version\'');
        if ($result === false) {
            $this->schema_version = false;
        } else {
            $a = mysql_fetch_array($result);
            $this->schema_version = $a['value'];
        }

        //assign the connection
        $this->sql_connection = $con;

        return true;
    }

    public function getSchemaVersion() {
        return $this->schema_version;
    }

    public function isTable($mixed) {
        return in_array($mixed, $this->sql_tables);
    }

    public function __destruct() {
        if(is_resource($this->last_query))
            mysql_free_result($this->last_query);
        mysql_close($this->sql_connection);
    }

    //Creates tables
    public function setup() {
        $tables = explode(';', file_get_contents('documents/shema.mysql'));
        foreach ($tables as $table) mysql_query($table);
    }

    //Perform simple SQL query
    public function myQuery($q) {
        if(is_resource($this->last_query))
            mysql_free_result($this->last_query);
        $this->last_query = mysql_query($q, $this->sql_connection);
        $this->last_insert_id = mysql_insert_id();
        if ($this->last_query === false) {
            $this->analyze();
            throw new DatabaseQueryException('Error #'.mysql_errno().':'.mysql_error(), $q);
        }
        return $this->last_query;
    }

    //True is last query failed
    public function failed() {
        if ($this->last_query === false) return true;
        return false;
    }

    //Returns current array with data values
    public function fetch_array() {
        if (!$this->failed())
            if (isset($this->last_query))
                return mysql_fetch_array($this->last_query);
            else
                throw new ClassException('Attempt to fetch a null query.');
        else
            throw new ClassException('Attempt to fetch failed query.');
    }

    //Returns the last insert id
    public function insert_id() {
        return $this->last_insert_id;
    }

    //Returns the number of rows affected
    public function num_rows() {
        if (!$this->failed())
            return mysql_num_rows($this->last_query);
        else
            throw new ClassException('Attempt to count failed query.');
    }

    //Quotes a string
    public function escape_string($string) {
        return mysql_real_escape_string($string);
    }

    //Quotes a value so it's safe to use in SQL statement
    public function quote($value) {
        if(is_numeric($value) && $value[0] != '0')
            return (int) $value;
        else
            return '\''.$this->escape_string($value).'\'';
    }

    public function analyze() {
    //determine database type, try to perform autosetup
        $is_aac_db = in_array('nicaw_accounts',$this->sql_tables);
        $is_server_db = in_array('accounts',$this->sql_tables) && in_array('players',$this->sql_tables);
        $is_svn = in_array('player_depotitems',$this->sql_tables) && in_array('groups',$this->sql_tables);
        $is_cvs = in_array('playerstorage',$this->sql_tables) && in_array('skills',$this->sql_tables);
        if (!$is_aac_db) {
            $this->setup();
            throw new DatabaseException('Notice: AutoSetup has attempted to create missing tables for you. Please create MySQL tables manually from "database.sql" if you are still getting this message.', 3);
        }elseif (!$is_server_db) {
            throw new DatabaseException('It appears you don\'t have SQL sample imported for OT server or it is not supported.', 4);
        }elseif ($is_cvs && !$is_svn) {
            throw new DatabaseException('This AAC version does not support your server. Consider using SQL v1.5.', 5);
        }
        return true;
    }

    public function repairTables() {
        if (isset($this->sql_tables))
            foreach($this->sql_tables as $table)
                mysql_query('REPAIR TABLE '.$table);
        return true;
    }

    ######################################
    # Methods for simple  data access    #
    ######################################

    //Insert data
    public function myInsert($table,$data) {global $cfg;
        $fields = array_keys($data);
        $values = array_values($data);
        $query = 'INSERT INTO `'.mysql_real_escape_string($table).'` (';
        foreach ($fields as $field)
            $query.= '`'.mysql_real_escape_string($field).'`,';
        $query = substr($query, 0, strlen($query)-1);
        $query.= ') VALUES (';
        foreach ($values as $value)
            if ($value === null)
                $query.= 'NULL,';
            else
                $query.= $this->quote($value).',';
        $query = substr($query, 0, strlen($query)-1);
        $query.= ');';
        $this->myQuery($query);
        return true;
    }

    //Replace data
    public function myReplace($table,$data) {global $cfg;
        $fields = array_keys($data);
        $values = array_values($data);
        $query = 'REPLACE INTO `'.mysql_real_escape_string($table).'` (';
        foreach ($fields as $field)
            $query.= '`'.mysql_real_escape_string($field).'`,';
        $query = substr($query, 0, strlen($query)-1);
        $query.= ') VALUES (';
        foreach ($values as $value)
            if ($value === null)
                $query.= 'NULL,';
            else
                $query.= $this->quote($value).',';
        $query = substr($query, 0, strlen($query)-1);
        $query.= ');';
        $this->myQuery($query);
        return true;
    }

    //Retrieve single row
    public function myRetrieve($table,$data) {
        $fields = array_keys($data);
        $values = array_values($data);
        $query = 'SELECT * FROM `'.mysql_real_escape_string($table).'` WHERE (';
        for ($i = 0; $i < count($fields); $i++)
            $query.= '`'.mysql_real_escape_string($fields[$i]).'` = '.$this->quote($values[$i]).' AND ';
        $query = substr($query, 0, strlen($query)-4);
        $query.=');';
        $this->myQuery($query);
        if ($this->num_rows() != 1) return false;
        return $this->fetch_array();
    }

    //Update data
    public function myUpdate($table,$data,$where,$limit=1) {
        $fields = array_keys($data);
        $values = array_values($data);
        $query = 'UPDATE `'.mysql_real_escape_string($table).'` SET ';
        for ($i = 0; $i < count($fields); $i++)
            $query.= '`'.mysql_real_escape_string($fields[$i]).'` = '.$this->quote($values[$i]).', ';
        $query = substr($query, 0, strlen($query)-2);
        $query.=' WHERE (';
        $fields = array_keys($where);
        $values = array_values($where);
        for ($i = 0; $i < count($fields); $i++)
            $query.= '`'.mysql_real_escape_string($fields[$i]).'` = '.$this->quote($values[$i]).' AND ';
        $query = substr($query, 0, strlen($query)-4);
        if (isset($limit))
            $query.=') LIMIT '.$limit.';';
        else
            $query.=');';
        $this->myQuery($query);
        return true;
    }

    //Delete data
    public function myDelete($table,$data,$limit = 1) {
        $fields = array_keys($data);
        $values = array_values($data);
        $query = 'DELETE FROM `'.mysql_real_escape_string($table).'` WHERE (';
        for ($i = 0; $i < count($fields); $i++)
            $query.= '`'.mysql_real_escape_string($fields[$i]).'` = '.$this->quote($values[$i]).' AND ';
        $query = substr($query, 0, strlen($query)-4);
        if ($limit > 0)
            $query.=') LIMIT '.$limit.';';
        else
            $query.=');';
        $this->myQuery($query);
        return true;
    }
}
?>
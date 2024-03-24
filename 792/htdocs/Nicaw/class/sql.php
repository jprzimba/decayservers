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

class SQL{
private $last_query, $last_error, $last_insert_id;
protected $sql_tables;
private $sql_connection;

//creates new connection
public function __construct($server, $user, $password, $database){

    //warn if MySQL extension is not installed
    if(!extension_loaded('mysqli'))
        throw new aacException('MySQL library is not installed. Database access is impossible. '.AAC::HelpLink(0));

    //establish a link to MySQL
    $con = mysqli_connect($server,$user,$password,$database);
    if ($con === false){
        throw new aacException('Unable to connect to mysql server. Please make sure it is up and running and you have correct user/password in config.inc.php. '.AAC::HelpLink(1));
        return false;
    }

    //select otserv database
    if (!mysqli_select_db($con, $database)){
        throw new aacException('Unable to select database: '.$database.'. Make sure it exists. '.AAC::HelpLink(2));
        return false;
    }

    //retrieve table list
	$result = mysqli_query($con, 'SHOW TABLES');
	if ($result === false) return false;
	while ($a = mysqli_fetch_array($result))
	$this->sql_tables[] = $a[0];

    //assign the connection
    $this->sql_connection = $con;

	return true;
}

public function isTable($mixed) {
    return in_array($mixed, $this->sql_tables);
}

public function __destruct(){
    if(is_resource($this->last_query))
        mysql_free_result($this->last_query);
    mysqli_close($this->sql_connection);
}

//Creates tables
public function setup(){
	$tables = explode(';', file_get_contents('documents/shema.mysql'));
	foreach ($tables as $table) mysqli_query($this->sql_connection, $table);
}

//Perform simple SQL query
public function myQuery($q){
    if(is_resource($this->last_query))
        mysql_free_result($this->last_query);
	$this->last_query = mysqli_query($this->sql_connection, $q);
    $this->last_insert_id = mysqli_insert_id($this->sql_connection);
	if ($this->last_query === false){
		$this->last_error = 'Error #'.mysqli_errno($this->sql_connection)."\n".$q."\n" . mysqli_error($this->sql_connection) . "\n";
		$analysis = $this->analyze();
		if ($analysis !== false)
			throw new aacException($analysis."\n".$this->last_error);
	}
	return $this->last_query;
}

//True is last query failed
public function failed(){
    if ($this->last_query === false) return true;
	return false;
}

//Returns current array with data values
public function fetch_array(){
    if (!$this->failed())
        if (isset($this->last_query))
            return mysqli_fetch_array($this->last_query);
        else
            throw new aacException('Attempt to fetch a null query.');
    else
      throw new aacException('Attempt to fetch failed query'."\n".$this->last_error);
}

//Returns the last insert id
public function insert_id(){
      return $this->last_insert_id;
}
  
//Returns the number of rows affected
public function num_rows()
{
    if (!$this->failed()) {
        return mysqli_num_rows($this->last_query);
    } else {
        throw new aacException('Attempt to count failed query'."\n".$this->last_error);
    }
}


//Quotes a string
public function scapeString($string)
{
    return mysqli_real_escape_string($this->sql_connection, $string);
}

//Quotes a value so it's safe to use in SQL statement
public function quote($value)
{
    if (is_numeric($value) && strpos($value, '.') === false && strpos($value, ',') === false) {
        return (int)$value;
    } else {
        return '\'' . $this->scapeString($value) . '\'';
    }
}

//Return last error
public function getError()
	{
		return $this->last_error;
	}
	
public function analyze()
	{
		//determine database type, try to perform autosetup
		$is_aac_db = in_array('nicaw_accounts',$this->sql_tables);
		$is_server_db = in_array('accounts',$this->sql_tables) && in_array('players',$this->sql_tables);
		$is_svn = in_array('player_depotitems',$this->sql_tables) && in_array('groups',$this->sql_tables);
		$is_cvs = in_array('playerstorage',$this->sql_tables) && in_array('skills',$this->sql_tables);
		if (!$is_aac_db){
			$this->setup();
			return 'Notice: AutoSetup has attempted to create missing tables for you. Please create MySQL tables manually from "database.sql" if you are still getting this message. '.AAC::HelpLink(3);
		}elseif (!$is_server_db){
			return 'It appears you don\'t have SQL sample imported for OT server or it is not supported. '.AAC::HelpLink(4);
		}elseif ($is_cvs && !$is_svn){
			return 'This AAC version does not support your server. Consider using SQL v1.5 '.AAC::HelpLink(5);
		}return false;
	}
	
public function repairTables()
	{
		if (isset($this->sql_tables))
			foreach($this->sql_tables as $table)
				mysql_query('REPAIR TABLE '.$table);
		return $return;
	}

######################################
# Methods for simple  data access    #
######################################

//Insert data
public function myInsert($table,$data)
	{global $cfg;
		$fields = array_keys($data);
		$values = array_values($data);
		$query = 'INSERT INTO `'.mysqli_escape_string($this->sql_connection, $table).'` (';
		foreach ($fields as $field)
			$query.= '`'.mysqli_escape_string($this->sql_connection, $field).'`,';
		$query = substr($query, 0, strlen($query)-1);
		$query.= ') VALUES (';
		foreach ($values as $value)
			if ($value === null)
				$query.= 'NULL,';
			else
				$query.= $this->quote($value).',';
		$query = substr($query, 0, strlen($query)-1);
		$query.= ');';
		if ($this->myQuery($query) === false) 
			return false;
		else
			return true;

	}
	
//Replace data
public function myReplace($table,$data)
	{global $cfg;
		$fields = array_keys($data);
		$values = array_values($data);
		$query = 'REPLACE INTO `'.mysqli_escape_string($this->sql_connection, $table).'` (';
		foreach ($fields as $field)
			$query.= '`'.mysqli_escape_string($this->sql_connection, $field).'`,';
		$query = substr($query, 0, strlen($query)-1);
		$query.= ') VALUES (';
		foreach ($values as $value)
			if ($value === null)
				$query.= 'NULL,';
			else
				$query.= $this->quote($value).',';
		$query = substr($query, 0, strlen($query)-1);
		$query.= ');';
		if ($this->myQuery($query) === false) 
			return false;
		else
			return true;

	}
	
//Retrieve single row
public function myRetrieve($table, $data)
{
    $fields = array_keys($data);
    $values = array_values($data);
    $query = 'SELECT * FROM `' . $this->scapeString($table) . '` WHERE (';
    for ($i = 0; $i < count($fields); $i++) {
        $query .= '`' . $this->scapeString($fields[$i]) . '` = ' . $this->quote($values[$i]) . ' AND ';
    }
    $query = rtrim($query, ' AND ');
    $query .= ');';
    $this->myQuery($query);
    if ($this->failed()) {
        return false;
    }
    if ($this->num_rows() <= 0) {
        return false;
    }
    if ($this->num_rows() > 1) {
        throw new aacException('Unexpected SQL answer. More than one item exists.');
    }
    return $this->fetch_array();
}


//Update data
public function myUpdate($table,$data,$where,$limit=1)
	{
		$fields = array_keys($data); 
		$values = array_values($data);
		$query = 'UPDATE `'.mysqli_escape_string($this->sql_connection, $table).'` SET ';
		for ($i = 0; $i < count($fields); $i++)
			$query.= '`'.mysqli_escape_string($this->sql_connection, $fields[$i]).'` = '.$this->quote($values[$i]).', ';
		$query = substr($query, 0, strlen($query)-2);
		$query.=' WHERE (';
		$fields = array_keys($where); 
		$values = array_values($where);
		for ($i = 0; $i < count($fields); $i++)
			$query.= '`'.mysqli_escape_string($this->sql_connection, $fields[$i]).'` = '.$this->quote($values[$i]).' AND ';
		$query = substr($query, 0, strlen($query)-4);
		if (isset($limit))
			$query.=') LIMIT '.$limit.';';
		else
			$query.=');';
		$this->myQuery($query);
		if ($this->failed()) return false;
		return true;
	}

//Delete data
public function myDelete($table,$data,$limit = 1)
	{
		$fields = array_keys($data); 
		$values = array_values($data);
		$query = 'DELETE FROM `'.mysqli_escape_string($this->sql_connection, $table).'` WHERE (';
		for ($i = 0; $i < count($fields); $i++)
			$query.= '`'.mysqli_escape_string($this->sql_connection, $fields[$i]).'` = '.$this->quote($values[$i]).' AND ';
		$query = substr($query, 0, strlen($query)-4);
		if ($limit > 0)
			$query.=') LIMIT '.$limit.';';
		else
			$query.=');';
		$this->myQuery($query);
		if ($this->failed()) return false;
		return true;
	}
}
?>
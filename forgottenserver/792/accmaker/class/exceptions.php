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

class aacException extends Exception {
    private $help_id;

    public function __construct($msg = null, $help_id = null) {
        parent::__construct($msg);
        $this->help_id = $help_id;
    }

    public function getHelpId() {
        return $this->help_id;
    }

    public function getHelpLink() {
        return '<a href="http://aac.nicaw.net/help.php?issue='.$this->help_id.'">More Info</a>';
    }
}

class LibraryMissingException extends aacException {}
class ClassException extends aacException {}
class ModuleException extends aacException {}

class DatabaseException extends ClassException {}
class DatabaseQueryException extends DatabaseException {
    private $query_str;

    public function __construct($msg = null, $query_str = null, $help_id = null) {
        parent::__construct($msg, $help_id);
        $this->query_str = $query_str;
    }

    public function getQueryStr() {
        return $this->query_str;
    }
}
class DatabaseConnectException extends DatabaseException {}
class DatabaseSelectException extends DatabaseException {}

class GuildException extends ClassException {}
class RankNotFoundException extends GuildException {}
class RankIsUsedException extends GuildException {}
class GuildNotFoundException extends GuildException {}
class GuildNotLoadedException extends GuildException {}
class MemberExistsException extends GuildException {}

class AccountException extends ClassException {}
class AccountNotFoundException extends AccountException {}
class AccountNotLoadedException extends AccountException {}

class PlayerException extends ClassException {}
class PlayerNotFoundException extends PlayerException {}
class PlayerNotLoadedException extends PlayerException {}
class PlayerIsOnlineException extends PlayerException {}

class FormException extends ClassException {}
class FormNotFoundException extends FormException {}
?>
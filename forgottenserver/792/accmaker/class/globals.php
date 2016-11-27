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

class AAC {
    static public $SQL;

    static public function ValidPlayerName($name) {global $cfg;
        foreach ($cfg['invalid_names'] as $baned_name)
            if (preg_match('/'.$baned_name.'/i',$name))
                return false;
        return preg_match("/^[A-Z][a-z]{1,20}([ '-][A-Za-z][a-z]{1,15}){0,3}$/",$name)
            && strlen($name) <= 25 && strlen($name) >= 4
            && !file_exists($cfg['dirdata'].'monster/'.$name.'.xml')
            && !file_exists($cfg['dirdata'].'npc/'.$name.'.xml');
    }

    static public function ValidPassword($pass) {
        return strlen($pass) > 5 && strlen($pass) <= 50 && preg_match('/^[a-zA-Z0-9~!@#%&;,:\\\^\$\.\|\?\*\+\(\)]*$/',$pass);
    }

    static public function ValidAccountName($n) {
        return preg_match('/^[1-9][0-9]{5,7}$/',$n);
    }

    static public function ValidEmail($email) {
        return preg_match('/^[A-Z0-9._%-]+@[A-Z0-9._%-]+\.[A-Z]{2,4}$/i',$email);
    }

    static public function ValidGuildName($name) {
        return preg_match("/^[A-Z][a-z]{1,20}([ '-][A-Za-z][a-z]{1,15}){0,3}$/",$name);
    }

    static public function ValidGuildRank($name) {
        return preg_match('/^([A-Za-z0-9]+[ \'\-]?){1,4}$/',$name) && strlen($name) <= 20;
    }

    static public function ValidGuildNick($name) {
        return preg_match('/^([A-Za-z0-9!@#$%*]+[ \'\-]?)+$/',$name) && strlen($name) <= 20;
    }

    static public function getExperienceByLevel($lvl) {
        return floor(50*($lvl-1)*($lvl*$lvl-5*$lvl+12)/3);
    }

    static public function ExceptionHandler($exception) {
        global $cfg;
        echo '<pre style="position: absolute; top: 0px; left: 0px; background-color: white; color: black; border: 3px solid red;">';
        echo '<b>Fatal error:</b> Uncaught exception \''.get_class($exception).'\'<br/>';
        echo '<b>Message:</b> '.$exception->getMessage().'<br/>';
        if (($exception instanceof aacException) && $exception->getHelpId()) {
            echo '<b>Online help:</b> '.$exception->getHelpLink().'</b><br/>';
        }
        if (($exception instanceof DatabaseQueryException)) {
            echo '<b>SQL query:</b> '.$exception->getQueryStr().'</b><br/>';
        }
        echo '<b>File:</b> '.basename($exception->getFile()).' on line: '.$exception->getLine().'</b><br/>';
        echo 'Script was terminated because something unexpected happened. You can report this, if you think it\'s a bug.<br/>';
        echo '<br/><b>Debug Backtrace:</b>';
        if ($cfg['debug_backtrace']) {
            echo '<br/>';
            print_R(debug_backtrace());
        } else {
            echo 'Disabled';
        }
    }
}

/*
	this is leftover from older versions
	TODO: clean it up
*/

function strToDate($str) {
    $pieces = explode('-',$str);
    return mktime(0,0,0,(int)$pieces[1],(int)$pieces[2],(int)$pieces[0]);
}

function getVocLvl($voc) {
    global $cfg;
    return floor($cfg['vocations'][$voc]['level']);
}

function getVocExp($voc) {
    global $cfg;
    $x = $cfg['vocations'][$voc]['level'];
    return round(50*($x-1)*(pow($x,2)-5*$x+12)/3);
}

function getStyle($seed) {
    if ($seed % 2 == 0)
        return 'class="color1"';
    else
        return 'class="color2"';
}

function percent_bar($part, $total) {
    if ($total <= 0) return 'unknown';
    $percent = round($part/$total*100);
    if ($percent >= 10)
        $percent_text = $percent.'%';
    else
        $percent_text = '';
    return '<div class="percent_bar" style="width:'.($percent*2).'px">'.$percent_text.'</div>';
}
?>
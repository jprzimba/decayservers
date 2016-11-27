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
$ptitle= "Polls - $cfg[server_name]";
include ("header.inc.php");
?>
<div id="content">
<div class="top">Polls</div>
<div class="mid">
<?php 
$sql = AAC::$SQL;
if (isset($_GET['id']))
	$params = 'AND nicaw_polls.id = '.$sql->quote((int)$_GET['id']);
elseif (isset($_GET['show_all']))
	$params = '';
else{
	echo 'Only the active polls are displayed. [<a href="voting.php?show_all">Show all</a>]<br/><br/>';
	$params = 'AND nicaw_polls.startdate < UNIX_TIMESTAMP(NOW())
AND nicaw_polls.enddate + 604800 > UNIX_TIMESTAMP(NOW())';
}


//receives all polls, options and vote count =)
$query = 'SELECT a1.poll_id, a1.option_id, a1.question, a1.option, a1.minlevel, a1.hidden, COUNT( nicaw_poll_votes.option_id ) AS votes
FROM (
SELECT nicaw_polls.minlevel, nicaw_polls.hidden, nicaw_polls.id AS poll_id, nicaw_poll_options.id AS option_id, nicaw_polls.startdate, nicaw_polls.question, nicaw_poll_options.option
FROM nicaw_polls, nicaw_poll_options
WHERE nicaw_poll_options.poll_id = nicaw_polls.id '.
$params
.' ORDER BY nicaw_polls.startdate DESC
) AS a1
LEFT OUTER JOIN nicaw_poll_votes ON a1.option_id = nicaw_poll_votes.option_id
GROUP BY a1.option_id';
$sql->myQuery($query);

//sort the data by poll_id
while ($a = $sql->fetch_array()){
	$polls[$a['poll_id']]['question'] = $a['question'];
	$polls[$a['poll_id']]['minlevel'] = $a['minlevel'];
	if (!$a['hidden'])
		$polls[$a['poll_id']]['votes_total'] += $a['votes'];
	$polls[$a['poll_id']]['options'][$a['option_id']]['id'] = $a['option_id'];
	$polls[$a['poll_id']]['options'][$a['option_id']]['option'] = $a['option'];
	$polls[$a['poll_id']]['options'][$a['option_id']]['votes'] = $a['votes'];
}
if (isset($polls))
	foreach ($polls as $poll){
		echo '<b>'.htmlspecialchars($poll['question']).'</b><table style="width: 100%">';
		foreach ($poll['options'] as $option){
			echo '<tr><td class="vote_cell"><input type="radio" name="'.md5($poll['question']).'" onclick="if (confirm(\'Vote for &quot;'.addslashes(htmlspecialchars($option['option'])).'&quot; ?\')) ajax(\'form\',\'modules/vote.php\',\'option='.htmlspecialchars($option['id']).'\',true)"/></td><td style="width: 100px">'.htmlspecialchars($option['option']).'</td><td>'.percent_bar($option['votes'], $poll['votes_total']).'</td></tr>';
		}
		echo '</table>Total votes: '.$poll['votes_total'].'<br/><br/>';
	}
?>
</div>
<div class="bot"></div>
</div>
<?php include ("footer.inc.php");?>
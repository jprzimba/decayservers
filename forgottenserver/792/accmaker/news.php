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

if (isset($_GET['RSS2'])){
header("Content-type: application/rss+xml");

echo '<?xml version="1.0"?><rss version="2.0" xmlns:dc="http://purl.org/dc/elements/1.1/" xmlns:content="http://purl.org/rss/1.0/modules/content/"><channel><title>'.htmlspecialchars($cfg['server_name']).' News</title><link>'.htmlspecialchars($cfg['server_url']).'</link><description>Server news contains latest information about updates, downtimes and events.</description>';

$mysql = AAC::$SQL;
$sql = $mysql->myQuery('SELECT * FROM `nicaw_news` ORDER BY `date` DESC LIMIT 10');

while ($a = $mysql->fetch_array()){
  echo '<item>';
  echo '<guid>http://'.htmlspecialchars($cfg['server_url'].$_SERVER['PHP_SELF'].'?id='.$a['id']).'</guid>';
  echo '<title>'.htmlspecialchars($a['title']).'</title>';
  echo '<pubDate>'.date('D, d M Y H:i:s O',$a['date']).'</pubDate>';
  echo '<dc:creator>'.htmlspecialchars($a['creator']).'</dc:creator>';
  if ((bool)(int)$a['html']){
    echo '<content:encoded>'.htmlspecialchars($a['text']).'</content:encoded>';
  }else{
    require_once('class/simple_bb_code.php');
    $bb = new Simple_BB_Code();
    echo '<content:encoded>'.htmlspecialchars($bb->parse($a['text'])).'</content:encoded>';
  }
  echo '</item>';
}
echo '</channel></rss>';

}else{
$ptitle= "News - $cfg[server_name]";
include ("header.inc.php");
?>
<div id="content">
<div class="top">Server News</div>
<div class="mid">
<a href="news.php?RSS2" style="text-decoration: none; float: right;"><img src="resource/feed.png" title="Subscribe to RSS" alt="rss" style="vertical-align: middle;"/></a>
<?php 
$mysql = AAC::$SQL;
if (isset($_GET['id']))
	$mysql->myQuery('SELECT * FROM `nicaw_news` WHERE `id` = \''.mysql_escape_string((int)$_GET['id']).'\'');
else
	$mysql->myQuery('SELECT * FROM `nicaw_news` ORDER BY `date` DESC LIMIT 10');

while ($a = $mysql->fetch_array()){
  echo '<i>'.date("jS F Y",$a['date']).'</i>';
  echo ' - <b>'.htmlspecialchars($a['creator']).'</b>';
  echo '<h2>'.htmlspecialchars($a['title']).'</h2>';
  echo '<blockquote>';
  if ((bool)(int)$a['html']){
    echo $a['text'];
  }else{
    require_once('class/simple_bb_code.php');
    $bb = new Simple_BB_Code();
    echo $bb->parse($a['text']);
  }
  echo '</blockquote>';
  echo '<br/><br/>';
}
?>
</div>
<div class="bot"></div>
</div>
<?php include ("footer.inc.php");}?>
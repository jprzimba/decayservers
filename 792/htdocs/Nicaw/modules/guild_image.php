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
include ("../include.inc.php");
$guild = new Guild();
if (isset($_POST['image_submit'])){
	//check if user is guild owner
	if (!($guild->load($_REQUEST['guild_id']) && $guild->attrs['owner_acc'] == $_SESSION['account'] && !empty($_SESSION['account']))){
		$msg = new IOBox('msg');
		$msg->addMsg('Please login. Cannot load guild or it\'s not yours.');
		$msg->addClose('OK');
		$msg->show();
		die();
	}
	if ($_FILES['image']['size'] <= 102400){
		if ($_FILES['image']['error'] != 0) throw new aacException('Unknown error');
		if (!is_uploaded_file($_FILES['image']['tmp_name'])) throw new aacException('File is not uploaded via HTTP POST');
		if ($_FILES['image']['type'] == 'image/gif'){
			@unlink('guilds/'.$guild->attrs['id'].'.gif');
			copy($_FILES['image']['tmp_name'],'../guilds/'.$guild->attrs['id'].'.gif');
		}elseif (extension_loaded('gd')){
			if ($_FILES['image']['type'] == 'image/png')
				$im = @imagecreatefrompng($_FILES['image']['tmp_name']);
			elseif ($_FILES['image']['type'] == 'image/jpeg')
				$im = @imagecreatefromjpeg($_FILES['image']['tmp_name']);
			else die('Unsupported image type');
			if ($im === false) die('Unsupported image type');
			$_im = imagecreatetruecolor(64, 64);
			imagefill($_im, 0, 0, imagecolorallocate($_im, 255, 255, 255));
			$x = imagesx($im);
			$y = imagesy($im);
			if ($x > $y){
				$_x = 64;
				$_y = $y/$x*64;
			}else{
				$_y = 64;
				$_x = $x/$y*64;
			}
			imagecopyresampled($_im, $im, (64-$_x)/2, (64-$_y)/2, 0, 0, $_x, $_y, $x, $y);
			imagegif($_im, '../guilds/'.$guild->attrs['id'].'.gif');
		}
	}else die('Image too big');
	header('location: '.$_SERVER['HTTP_REFERER']);
}else{
?>
<div id="iobox" class="draggable">
<fieldset><legend>Upload Image</legend>
<form method="post" action="modules/guild_image.php?guild_id=<?php echo (int)$_REQUEST['guild_id']; ?>" enctype="multipart/form-data">
<input type="file" name="image">
<input type="Submit" name="image_submit" value="Upload">
<input onclick="$('iobox').style['visibility'] = 'hidden'" type="button" value="Close"/>
<br/>
Supported type *.GIF *.JPEG *.PNG 64x64 100KB
</form></fieldset></div>
<?php } ?>
<?php
session_start();
function myImageBlur($im,$distance=1)
{
	// w00t. my very own blur function
	// in GD2, there's a gaussian blur function. bunch of bloody show-offs... :-)

	$width = imagesx($im);
	$height = imagesy($im);

	$temp_im = ImageCreateTrueColor($width,$height);
	$bg = ImageColorAllocate($temp_im,150,150,150);

	// preserves transparency if in orig image
	ImageColorTransparent($temp_im,$bg);

	// fill bg
	ImageFill($temp_im,0,0,$bg);

	// anything higher than 3 makes it totally unreadable
	// might be useful in a 'real' blur function, though (ie blurring pictures not text)
	// use $distance=30 to have multiple copies of the word. not sure if this is useful.

	// blur by merging with itself at different x/y offsets:
	$rand=rand(0,2);
	ImageCopyMerge($temp_im, $im, 0, 0, 0, $distance, $width, $height-$distance, 70);
	ImageCopyMerge($im, $temp_im, 0, 0, $distance, 0, $width-$distance, $height, 70);
	ImageCopyMerge($temp_im, $im, 0, $distance, 0, 0, $width, $height, 70);
	ImageCopyMerge($im, $temp_im, $distance, 0, 0, 0, $width, $height, 70);
	// remove temp image
	ImageDestroy($temp_im);

	return $im;
}

/*
** This function generates a picture with a text passed to it.
** 
** Written by Alexander Graf, 10/04/2002
** AlexGraf@web.de
*/

if (empty($_SESSION['RandomText'])) $_SESSION['RandomText'] = substr(str_shuffle(strtolower('qwertyuipasdfhjklzxcvnm12345789')), 0, 6);

// Create the image with width=150 and height=40
$width = 250;
$height = 40;
$IMGVER_IMAGE = imagecreate($width,$height);

// Allocate two colors (Black & White)
// This uses the RGB names of the colors
$IMGVER_COLOR_BACK = imagecolorallocate ($IMGVER_IMAGE, 255, 228, 157);
$IMGVER_COLOR_BACK = imagecolortransparent ($IMGVER_IMAGE, $IMGVER_COLOR_BACK);
$IMGVER_COLOR_TEXT = imagecolorallocate ($IMGVER_IMAGE, rand(0,255), rand(0,255), rand(0,255));

// Flood Fill our image with black
imagefill($IMGVER_IMAGE, 0, 0, $IMGVER_COLOR_BACK);

// Since our Text had 6 chars (we defined this not to be longer)
// we now write the 6 random chars in our picture
// For those who don�t know: You can access the third character
//in a string easily by typing $myString[2];
imagestring($IMGVER_IMAGE, imageloadfont ("resource/font1.gdf"), 0, rand(0,8), strtoupper($_SESSION['RandomText']) ,$IMGVER_COLOR_TEXT);


//Now we send the picture to the Browser
header("Content-type: image/gif");

// always modified
header("Last-Modified: " . gmdate("D, d M Y H:i:s") . " GMT");

// HTTP/1.1
header("Cache-Control: no-store, no-cache, must-revalidate");
header("Cache-Control: post-check=0, pre-check=0", false);

// HTTP/1.0
header("Pragma: no-cache");

//imageline( $IMGVER_IMAGE, rand(0,$width), rand(0,$height), rand(0,$width), rand(0,$height), $IMGVER_COLOR_TEXT);
$IMGVER_IMAGE = myImageBlur($IMGVER_IMAGE,30);
$IMGVER_IMAGE = myImageBlur($IMGVER_IMAGE,1);
imagegif($IMGVER_IMAGE);
?>
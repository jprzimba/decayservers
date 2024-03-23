<noscript class="exception">This website requires javascript to run correctly.</noscript><br/>
<div id="footer" class="clearer">&nbsp;<span>Powered by <a href="http://nicaw.net/">Nicaw AAC</a> &copy; 2007 under <a href="documents/LICENCE.TXT">GPL</a></span></div>
</div>
<?php 
//Get current time as we did at start
    $mtime = microtime();
    $mtime = explode(" ",$mtime);
    $mtime = $mtime[1] + $mtime[0];
//Store end time in a variable
    $tend = $mtime;
//Calculate the difference
    $totaltime = ($tend - $tstart);
//Output result
    printf ("<!--Page was generated in %f seconds !-->\n", $totaltime); 
?>
</body>
</html>
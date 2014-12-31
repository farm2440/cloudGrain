<html>
<head>
  <script type="text/JavaScript">
  <!--
   function timedRefresh(timeoutPeriod) 
   {
          setTimeout("location.reload(true);",timeoutPeriod);
   }
  //   -->
  </script>
</head>

<body onload="JavaScript:timedRefresh(5000);">
<p>This page will refresh every 5 seconds. </p>


 <?php
	$sensors = file_get_contents('sensors.html');
	echo $sensors;
  ?>
</body>
</html>

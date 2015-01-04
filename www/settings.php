<!DOCTYPE html>
<html>
<head>
<title>ThermoLog cGate - Live data</title>
<meta name="generator" content="Bluefish 2.2.5" >
<meta name="author" content="Svilen" >
<meta name="date" content="2015-01-04T21:40:54+0200" >
<meta name="copyright" content="">
<meta name="description" content="">
<meta http-equiv="content-type" content="text/html; charset=UTF-8">
<meta http-equiv="content-type" content="application/xhtml+xml; charset=UTF-8">
<meta http-equiv="content-style-type" content="text/css">
<meta http-equiv="expires" content="0">
<link href="cgate.css" rel="stylesheet" type="text/css">
<style type="text/css">

</style>
<script type="text/javascript">
<!--
   function timedRefresh(timeoutPeriod) 
   {
          setTimeout("location.reload(true);",timeoutPeriod);
   }
// -->
</script>
</head>


<body onload="JavaScript:timedRefresh(20000);">

	<div id="logo">
 		<img src="logo.jpeg" width="213" height="120" alt="LOGO" longdesc="Etherino logo">
	</div>
 	
	<div id="header">	 
		<h1>ThermoLog cGate</h1>
		firmware v.1.1.26 from 03/01/2015
	</div>

	<div id="login"><a>Login</a> </div>

	<div id="Nav">
	     <div id="nextNav">
	     		<a href="livedata.php">Live Data</a>
	     </div>
	     <div id="nextNav">
	     		<a href="livesensors.php">Sensors</a>
	     </div>
	     <div id="currentNav">Settings</div>
   </div>
   <div id="liveData"> 
	    <div id="siloId">
		    Silo: 3<br> 
		    Location: Зърнобаза Суворово<br>
	    </div>
	    <div id="liveDataTable">
		    <?php
				$sensors = file_get_contents('/mnt/ramdisk/settings.html');
				echo $sensors;
  			?>
  	   </div>
	</div>

	<div id="footer">
		<a href="www.etherino.net">www.etherino.net</a>
	</div>
</body>
</html>
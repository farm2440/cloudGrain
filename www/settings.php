<!DOCTYPE html>
<html>
<head>
<title>ThermoLog cGate - Live data</title>
<meta name="generator" content="Bluefish 2.2.5" >
<meta name="author" content="Svilen" >
<meta name="date" content="2015-01-07T17:35:39+0200" >
<meta name="copyright" content="">
<meta name="description" content="">
<meta http-equiv="content-type" content="text/html; charset=UTF-8">
<meta http-equiv="content-type" content="application/xhtml+xml; charset=UTF-8">
<meta http-equiv="content-style-type" content="text/css">
<meta http-equiv="expires" content="0">
<link href="cgate.css" rel="stylesheet" type="text/css">
<style type="text/css">
</style>
</head>


<body>
	<div id="logo">
 		<img src="logo.jpeg"  width="213" height="120" alt="LOGO" longdesc="Etherino logo">
	</div>
 	
	<div id="header">	 
		<h1>ThermoLog cGate</h1>
		<?php
			$sensors = file_get_contents('/mnt/ramdisk/version.html');
			echo $sensors;
  		?>
	</div>

	<div id="Nav">
	     <div id="nextNav">
	     		<a href="livedata.php">Live Data</a>
	     </div>
	     <div id="nextNav">
	     		<a href="sensorstable.php">Sensors</a>
	     </div>
	     <div id="currentNav">Settings</div>
   </div>
   <div id="DataSection"> 
	    <div id="TableContainer">
		    <?php
				$sensors = file_get_contents('/mnt/ramdisk/settings.html');
				echo $sensors;
  			?>
  	   </div>

		<?php
			$upload_enabled = file_get_contents('/sys/class/gpio/gpio48/value');
			if($upload_enabled==0) 
			{
				echo '<form action="upload.php" method="post" enctype="multipart/form-data">';
    			echo 'Select image to upload:';
    			echo '<input type="file" name="fileToUpload" id="fileToUpload">';
				echo '<input type="submit" value="Upload Image" name="submit">';
				echo '</form>';
			}
			else echo 'Configuration upload is not enabled by hardware. Check the manual for options';
		?>
		
  	   
	</div>

	<div id="footer">
		<a href="www.etherino.net">www.etherino.net</a>
	</div>
</body>
</html>
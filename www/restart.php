<?php
	session_start();
?>
<!DOCTYPE html>
<html>
<head>
<title>ThermoLog cGate - Settings upload</title>
<meta name="generator" content="Bluefish 2.2.5" >
<meta name="author" content="Svilen" >
<meta name="date" content="2015-01-18T10:38:34+0200" >
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
<table id="layoutTable">

	<tr>
		<td id="logo">
			<img src="logo.png" width="213" height="120" alt="LOGO" longdesc="Etherino logo">
 		</td>
 		<td id="header">
			<table id="layoutTable">
			<tr><td><img src="cGate-logo.png" width="213" height="60" alt="LOGO" longdesc="Etherino logo"></td></tr>
			<tr><td>				
				<?php
					$sensors = file_get_contents('/mnt/ramdisk/version.html');
					echo $sensors;
  				?>
			</td></tr>
			</table>
		</td>
	</tr>
	
	<tr>
		<td id="Nav">
	   	<div id="nextNav">
	     		<a href="index.php">Live Data</a>
	     	</div>
	     	<div id="nextNav">
	     		<a href="sensorstable.php">Sensors</a>
	     	</div>
	     	<div id="nextNav">
	     		<a href="settings.php">Settings</a>
	     	</div>
		</td>

		<td id="DataSection">
			<?php
				$upload_enabled = file_get_contents('/sys/class/gpio/gpio48/value');
				if($upload_enabled==0) 
				{
					if ($_SESSION['readyToReset']=='YES')
					{
						echo "Restarting. This will take about 60 seconds.";
					}
				}
				else echo 'Remote restart is locked by hardware.';
			?> 
		</td>
	</tr>

	<tr>
		<td id="footer" colspan="2">
				<a href="http://www.etherino.net">www.etherino.net</a>
		</td>
	</tr>
</table>
</body>
</html>

<?php
	if ($_SESSION['readyToReset']=='YES')
	{
		$_SESSION['readyToReset']='NO';
   	exec('bash -c "exec nohup setsid reboot > /dev/null 2>&1 &"');
	}
?>

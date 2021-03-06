<?php
	session_start();
?>

<!DOCTYPE html>
<html>
<head>
<title>ThermoLog cGate - Settings upload</title>
<meta name="generator" content="Bluefish 2.2.5" >
<meta name="author" content="Svilen" >
<meta name="date" content="2015-01-18T10:39:16+0200" >
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
			<img src="logo.jpeg" width="213" height="120" alt="LOGO" longdesc="Etherino logo">
 		</td>
 		<td id="header">
				<h1>ThermoLog cGate</h1>
				<?php
					$sensors = file_get_contents('/mnt/ramdisk/version.html');
					echo $sensors;
  				?>
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
				
				$target_dir = "/home/root/";
				$target_file = $target_dir . "settings.xml";
				$uploadOk = 1;
				$imageFileType = pathinfo($target_file,PATHINFO_EXTENSION);
				// Check if image file is a actual image or fake image
				if($_FILES["fileToUpload"]["tmp_name"]=='') 
				{
					echo 'No file selected!';
					$uploadOk = 0;
				}

				// Check file size
				if ($_FILES["fileToUpload"]["size"] > 500000) 
				{
    				echo "Sorry, your file is too large.";
    				$uploadOk = 0;
				}

				// Allow certain file formats
				if($imageFileType != "xml" ) 
				{
  				  	echo "Sorry, only xml files are allowed.";
					$uploadOk = 0;
				}
				// Check if $uploadOk is set to 0 by an error
				if ($uploadOk == 0) 
				{
    				echo "<br>Sorry, your file was not uploaded.";
					// if everything is ok, try to upload file
				} 
				else 
				{
    				if (move_uploaded_file($_FILES["fileToUpload"]["tmp_name"], $target_file)) 
    				{
        				echo "The file ". basename( $_FILES["fileToUpload"]["name"]). " has been uploaded.";
		  				echo "<p>cGate must restart to apply new settngs!</p>";  
		  				echo "<form action=\"restart.php\" method=\"post\">";
 		  				echo "<p><input type=\"submit\" value=\"RESTART NOW\"/></p></form>";      
 		  				$_SESSION['readyToReset']='YES';
    				} 
    				else 
    				{
						echo "Sorry, there was an error uploading your file.";
    				}
				}
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
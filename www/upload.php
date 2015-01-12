<!DOCTYPE html>
<html>
<head>
<title>ThermoLog cGate - Settings upload</title>
<meta name="generator" content="Bluefish 2.2.5" >
<meta name="author" content="Svilen" >
<meta name="date" content="2015-01-12T22:39:59+0200" >
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
	     <div id="nextNav">
	     		<a href="settings.php">Settings</a>
	     </div>
   </div>
   <div id="DataSection"> 
	    <div id="TableContainer">
<?php

if (isset($_POST['restart']))
{
	echo "Restarting...";
   exec("reboot");
}

$target_dir = "/home/root/";
$target_file = $target_dir . "settings.xml";
$uploadOk = 1;
$imageFileType = pathinfo($target_file,PATHINFO_EXTENSION);
// Check if image file is a actual image or fake image
if(isset($_POST["submit"])) {
    $check = getimagesize($_FILES["fileToUpload"]["tmp_name"]);
}

// Check file size
if ($_FILES["fileToUpload"]["size"] > 500000) {
    echo "Sorry, your file is too large.";
    $uploadOk = 0;
}
// Allow certain file formats
if($imageFileType != "xml" ) {
    echo "Sorry, only xml files are allowed.";
    $uploadOk = 0;
}
// Check if $uploadOk is set to 0 by an error
if ($uploadOk == 0) {
    echo "Sorry, your file was not uploaded.";
// if everything is ok, try to upload file
} else {
    if (move_uploaded_file($_FILES["fileToUpload"]["tmp_name"], $target_file)) {
        echo "The file ". basename( $_FILES["fileToUpload"]["name"]). " has been uploaded.";
		  echo "<p>cGate must restart to apply new settngs!</p>";
		  echo "<form action=\"\" method=\"post\">";
 		  echo "<p><input type=\"hidden\" name=\"restart\" value=\"yes\"/><input type=\"submit\" value=\"RESTART NOW\"/></p></form>";
        
    } else {
        echo "Sorry, there was an error uploading your file.";
    }
}
?> 
  	   </div>	   
	</div>

	<div id="footer">
		<a href="www.etherino.net">www.etherino.net</a>
	</div>
</body>
</html>
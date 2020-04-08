<?php
  date_default_timezone_set('Europe/Athens');
  
  $nul = "NULL";

  // Replace XXXXXX_XXXX with the name of the header you need in UPPERCASE (and with '-' replaced by '_') starting with HTTP_
  $mac = isset($_SERVER['HTTP_X_ESP_MAC']) ? $_SERVER['HTTP_X_ESP_MAC'] : $nul;
  $psw = isset($_REQUEST['psw']) ? $_REQUEST['psw'] : $nul;
    
	if ($psw == $nul || $mac == $nul || $psw != "1234") {
		echo "Wrong data.";
    http_response_code(401);  //unauthorised
		return false;  
	}
  
  $received = file_get_contents('php://input');
  $time = strftime("%Y-%m-%d %H:%M:%S", time());
  $fileToWrite = $mac."-debug-".$time.".txt";
  file_put_contents($fileToWrite, $received);
  http_response_code(200);
?>
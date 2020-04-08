#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPCrashSave.h>

ESPCrashSave crashSave;

// location of debug.php file on webserver
String post_url = "http://testsite.tld/subdir/debug.php";  

void setup(void) {
  Serial.begin(115200);
  Serial.println();
  Serial.println("ESP8266 crash log app.");

  WiFi.begin("Support", "12345678");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  // if there are crashes written in log, print them and send to web
  if (crashSave.crashLogFileExists()) {
    crashSave.printCrashLog();
    crashSave.sendCrashLogToWeb(post_url, "1234");
  }

  Serial.println();
  Serial.println("Press a key + <enter>");
  Serial.println("0 : attempt to divide by zero");
  Serial.println("e : attempt to read through a pointer to no object");
  Serial.println("c : clear crash information");
  Serial.println("p : print crash information");
  Serial.println("b : store crash information to buffer and print buffer");
  Serial.println();
}

void loop(void) {
  if (Serial.available() > 0) {
    char inChar = Serial.read();
    switch (inChar) {
      case '0':
        Serial.println("Attempting to divide by zero ...");
        int result, zero;
        zero = 0;
        result = 1 / zero;
        Serial.print("Result = ");
        Serial.println(result);
        break;
      case 'e':
        Serial.println("Attempting to read through a pointer to no object ...");
        int *nullPointer;
        nullPointer = NULL;
        // null pointer dereference - read
        // attempt to read a value through a null pointer
        Serial.print(*nullPointer);
        break;
      case 'c':
        crashSave.clearCrashLog();
        Serial.println("Crash information cleared");
        break;
      case 'p':
        Serial.println("--- BEGIN of crash info ---");
        crashSave.printCrashLog();
        Serial.println("--- END of crash info ---");
        break;
      case 0xa:
      case 0xd:
        // skip newline and carriage return
        break;
      default:
        Serial.printf("%c typed.\n", inChar);
        break;
    }
  }
}

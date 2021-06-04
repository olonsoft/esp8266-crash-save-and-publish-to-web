#include <ESPCrashSave.h>
#include <LittleFS.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>

/**
 * This function is called automatically if ESP8266 suffers an exception
 * It should be kept quick / consise to be able to execute before hardware wdt
 * may kick in
 *
 * Without writing to LittleFS this function take 2-3ms
 * Writing to flash only takes 10-11ms.
 * This complete function should be finised in 15-20ms
 */

String _filename = "crash.log";

extern "C" void custom_crash_callback(struct rst_info* rst_info, uint32_t stack,
                                      uint32_t stack_end) {
  uint32_t crashTime = millis();
  uint32_t savedSize = 0;
  // create loop iterators
  int16_t i;
  uint8_t j;

  FSInfo fs_info;
  // fill FSInfo struct with informations about the LittleFS
  LittleFS.info(fs_info);
  // if the remaining space is less than the length of the content to save
  uint32_t freeSpace = fs_info.totalBytes - fs_info.usedBytes;
  if (freeSpace < 512) return;  // todo

  // open the file in appending mode
  File _file = LittleFS.open(_filename, "a");

  // if the file does not yet exist
  if (!_file) {
    // open the file in write mode
    _file = LittleFS.open(_filename, "w");
  }

  // if the file is (now) a valid file
  if (_file) {
    // one complete log has 170 chars + 45 * n
    // n >= 2 e [2, 4, 6, ...]
    // 4220 + safety will last for 90 stack traces including header (170)
    // uint16_t maximumFileContent = 4250;

    // maximum tmpBuffer size needed is 83, so 100 should be enough
    char buffer[100];
    // max. 65 chars of Crash time, reason, exception
    snprintf(buffer, sizeof(buffer),
             "Crashed at %d ms\nRestart reason: %d\nException (%d):\n",
             crashTime, rst_info->reason, rst_info->exccause);
    savedSize = _file.write(buffer, strlen(buffer));

    // 83 chars of epc1, epc2, epc3, excvaddr, depc info + 13 chars of >stack>
    snprintf(buffer, sizeof(buffer),
             "epc1=0x%08x epc2=0x%08x epc3=0x%08x excvaddr=0x%08x "
             "depc=0x%08x\n>>>stack>>>\n",
             rst_info->epc1, rst_info->epc2, rst_info->epc3, rst_info->excvaddr,
             rst_info->depc);
    savedSize += _file.write(buffer, strlen(buffer));

    int16_t stackLength = stack_end - stack;
    uint32_t stackTrace;

    // collect stack trace
    // one loop contains 45 chars of stack address and its content
    // e.g. "3fffffb0: feefeffe feefeffe 3ffe8508 40100459"
    for (i = 0; i < stackLength; i += 0x10) {
      snprintf(buffer, sizeof(buffer), "%08x: ", stack + i);
      savedSize += _file.write(buffer, strlen(buffer));

      for (j = 0; j < 4; j++) {
        uint32_t* byteptr = (uint32_t*)(stack + i + j * 4);
        stackTrace = *byteptr;

        snprintf(buffer, sizeof(buffer), "%08x ", stackTrace);
        savedSize += _file.write(buffer, strlen(buffer));
      }

      savedSize += _file.write("\n", strlen("\n"));

      if (freeSpace - savedSize < 256) break;
    }

    snprintf(buffer, sizeof(buffer), "<<<stack<<<\n\n");
    savedSize += _file.write(buffer, strlen(buffer));

    _file.close();
  }
}

ESPCrashSave::ESPCrashSave() {
  LittleFS.begin();
}

void ESPCrashSave::setFilename(String filename) { _filename = filename; }

/*
 * @brief   Get remaining space of filesystem
 * @return  Free space in byte
 */
uint32_t ESPCrashSave::getFSFreeSpace() {
  FSInfo fs_info;

  // fill FSInfo struct with informations about the LittleFS
  LittleFS.info(fs_info);

  return (fs_info.totalBytes - (fs_info.usedBytes * 1.05));
}

bool ESPCrashSave::crashLogFileExists() {
  return LittleFS.exists(_filename);
}

void ESPCrashSave::printCrashLog() {

  if (!crashLogFileExists()) {
    Serial.println("Crash log file does not exist.");
    return;
  };

  File file = LittleFS.open(_filename, "r");
 
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.println("Printing Crash log:");

  while (file.available()) {
    Serial.write(file.read());
  }
 
  file.close();
}

/*
 * @brief   Deletes the crash log file
 * @return  True is successful
 */
bool ESPCrashSave::clearCrashLog() {
  if (LittleFS.exists(_filename)) {
    Serial.println("Deleting crash log file... ");
    if (LittleFS.remove(_filename)) {
      Serial.println("Success.");
      return true;
    } else {
      Serial.println("Fail.");
      return false;
    }
  }
  return false;
}

//todo https://github.com/esp8266/Arduino/issues/1872 

int ESPCrashSave::sendCrashLogToWeb(String url, String password) {

  String post_url = url + "?psw=" + password;

  WiFiClient client;
  HTTPClient http;
  http.begin(client, post_url);
  
  http.addHeader("Content-Type", "multipart/form-data");  
  http.addHeader(F("X-ESP-MAC"), WiFi.macAddress());
    
  File file = LittleFS.open(_filename, "r");

  if (!file) 
    return false;

  //CrashSave.print(_debugOutputBuffer, DEBUG_OUTPUT_SIZE);
  client.setTimeout(250);
  int httpCode = http.sendRequest("POST", &file, file.size());
  // httpCode will be negative on error 
  if (httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
    Serial.printf("[HTTP] POST/GET code: %d\n", httpCode);
  } else {
    Serial.printf("[HTTP] POST/GET... failed, error: %d %s\n", httpCode, http.errorToString(httpCode).c_str());
  } 
  http.end();  
  return httpCode;
}

#ifndef ESP_CRASH_SAVE_H
#define ESP_CRASH_SAVE_H

#include "Arduino.h"
#include <user_interface.h>

/*
 * Structure of the single crash data set
 *
 *  1. Crash time
 *  2. Restart reason
 *  3. Exception cause
 *  4. epc1
 *  5. epc2
 *  6. epc3
 *  7. excvaddr
 *  8. depc
 *  9. address of stack start
 * 10. address of stack end
 * 11. stack trace bytes
 *      ...
 */

class ESPCrashSave {
 public:
  ESPCrashSave();
  void setFilename(String filename);
  bool clearCrashLog();
  void printCrashLog();
  bool crashLogFileExists();
  int sendCrashLogToWeb(String url, String password);
  uint32_t getFSFreeSpace();
 private:  
  
};

#endif
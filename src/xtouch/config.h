#ifndef _XLCD_JSONCONFIG
#define _XLCD_JSONCONFIG

#include <ArduinoJson.h>
#include "filesystem.h"
#include "paths.h"

/*
{
  "ssid": "xperiments-2.4G",
  "pwd": "viernes13",
  "timeout": "3000",
  "coldboot": "5000",
  "mqtt": {
    "host": "192.168.0.18",
    "accessCode": "24660910",
    "serialNumber": "01S00C342600288",
    "printerModel": "P1P"
  }
}
*/
DynamicJsonDocument xtouch_load_config()
{
    DynamicJsonDocument config = xtouch_filesystem_readJson(SD, xtouch_paths_config);
    
    // Check if the JSON was loaded successfully and contains the required fields
    if (!config.isNull() && config.containsKey("mqtt")) {
        if (config["mqtt"].containsKey("accessCode")) {
            strcpy(xTouchConfig.xTouchAccessCode, config["mqtt"]["accessCode"].as<const char *>());
        }
        if (config["mqtt"].containsKey("serialNumber")) {
            strcpy(xTouchConfig.xTouchSerialNumber, config["mqtt"]["serialNumber"].as<const char *>());
        }
        if (config["mqtt"].containsKey("host")) {
            strcpy(xTouchConfig.xTouchHost, config["mqtt"]["host"].as<const char *>());
        }
        if (config["mqtt"].containsKey("printerModel")) {
            strcpy(xTouchConfig.xTouchPrinterModel, config["mqtt"]["printerModel"].as<const char *>());
        }
    }
    
    return config;
}

#endif
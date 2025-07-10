#include <driver/i2s.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include "xtouch/debug.h"
#include "xtouch/paths.h"
#include "xtouch/eeprom.h"
#include "xtouch/types.h"
#include "xtouch/bblp.h"
#include "xtouch/globals.h"
#include "xtouch/filesystem.h"
#include "ui/ui.h"
#include "xtouch/sdcard.h"
#include "xtouch/hms.h"

#if defined(__XTOUCH_SCREEN_28__)
#include "devices/2.8/screen.h"
#endif

#include "xtouch/settings.h"
#include "xtouch/net.h"
#include "xtouch/mqtt.h"
#include "xtouch/sensors/chamber.h"
#include "xtouch/events.h"
#include "xtouch/connection.h"
#include "xtouch/coldboot.h"

bool bIsOtaRunning = false;
bool bCrashedBefore;

void xtouch_intro_show(void)
{
    ui_introScreen_screen_init();
    lv_disp_load_scr(introScreen);
    lv_timer_handler();
}

void setup()
{
    esp_reset_reason_t reason = esp_reset_reason();
    switch (reason)
    {
        case ESP_RST_PANIC:      //!< Software reset due to exception/panic
        case ESP_RST_INT_WDT:    //!< Reset (software or hardware) due to interrupt watchdog
        case ESP_RST_TASK_WDT:   //!< Reset due to task watchdog
        case ESP_RST_WDT:        //!< Reset due to other watchdogs
        case ESP_RST_BROWNOUT:   //!< Brownout reset (software or hardware)
            bCrashedBefore = true;
            break;
        default:
            bCrashedBefore = false;
    }

#if XTOUCH_USE_SERIAL == true || XTOUCH_DEBUG_ERROR == true || XTOUCH_DEBUG_DEBUG == true || XTOUCH_DEBUG_INFO == true
    Serial.begin(115200);
#endif

    xtouch_eeprom_setup();
    xtouch_globals_init();
    xtouch_screen_setup();
    xtouch_intro_show();
    while (!xtouch_sdcard_setup())
        ;

    xtouch_coldboot_check();

    xtouch_settings_loadSettings();

    xtouch_touch_setup();

    while (!xtouch_wifi_setup())
        ;

    if (xTouchConfig.xTouchOTAEnabled)
    {
        DynamicJsonDocument otaConfig = xtouch_load_config();
        const char *hostName = otaConfig.containsKey("OtaHostName") ? otaConfig["OtaHostName"].as<const char *>() : OTADEFAULTHOSTNAME;
        const char *password = otaConfig.containsKey("OtaPassword") ? otaConfig["OtaPassword"].as<const char *>() : OTADEFAULTPASSWORD;
        ArduinoOTA.setHostname(hostName).setPassword(password).setMdnsEnabled(true).onStart([]()
                                                                                            {
                                String type;
                                if (ArduinoOTA.getCommand() == U_FLASH) type = "sketch";
                                else {
                                    type = "filesystem";
                                    //LittleFS.end();
                                }
                                bIsOtaRunning = true;
                                ConsoleInfo.printf("Start updating - %s \n", type.c_str()); })
            .onEnd([]()
                   { bIsOtaRunning = false; })
            .onProgress([](unsigned int progress, unsigned int total)
                        {
                            // Log.info("Progress: %u%%\r", (progress / (total / 100)));
                        })
            .onError([](ota_error_t error)
                     {
                                bIsOtaRunning = false;;
                                ConsoleError.printf("OTA Error[%u]: ", error);
                                if (error == OTA_AUTH_ERROR)  ConsoleError.println(F("Auth Failed\n"));
                                else if (error == OTA_BEGIN_ERROR) ConsoleError.println(F("Begin Failed\n"));
                                else if (error == OTA_CONNECT_ERROR) ConsoleError.println(F("Connect Failed\n"));
                                else if (error == OTA_RECEIVE_ERROR) ConsoleError.println(F("Receive Failed\n"));
                                else if (error == OTA_END_ERROR) ConsoleError.println(F("End Failed\n")); });

        ArduinoOTA.begin();
        ConsoleInfo.printf("OTA updates enabled, hostname: %s password: %s\n", hostName, password);
        if (bCrashedBefore)
        {
            // Avoid crash loops where OTA updates can't be applied, power off->on will get back to the normal boot sequence
            ConsoleInfo.println(F("Reboot due to crash. Safe Mode, waiting for OTA update.\n")); 
            lv_label_set_text(introScreenCaption, LV_SYMBOL_WARNING  " OTA-only mode after crash");
            lv_obj_set_style_text_color(introScreenCaption, lv_color_hex(0xff0000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_timer_handler();
            lv_task_handler();
            delay(32);
            return;
        }
    }
    else
    {
        ConsoleInfo.println(F("OTA updates disabled\n"));
    }

    xtouch_screen_setupScreenTimer();
    xtouch_setupGlobalEvents();

    xtouch_mqtt_setup();
    xtouch_chamber_timer_init();
}

void loop()
{
    ArduinoOTA.handle();
    if (bIsOtaRunning || (bCrashedBefore && xTouchConfig.xTouchOTAEnabled))
    {
        // Do not continue regular operation as long as a OTA is running
        delay(50);
        return;
    }

    
    lv_timer_handler();
    lv_task_handler();
    xtouch_mqtt_loop();
}

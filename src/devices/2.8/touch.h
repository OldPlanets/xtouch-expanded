#ifndef _XLCD_TOUCH
#define _XLCD_TOUCH

#include <XPT2046_Touchscreen.h>

#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

SPIClass x_touch_spi = SPIClass(HSPI);
XPT2046_Touchscreen x_touch_touchScreen(XPT2046_CS, XPT2046_IRQ);
XTouchPanelConfig x_touch_touchConfig;

class ScreenPoint
{
public:
    int16_t x;
    int16_t y;

    // default constructor
    ScreenPoint()
    {
    }

    ScreenPoint(int16_t xIn, int16_t yIn)
    {
        x = xIn;
        y = yIn;
    }
};

ScreenPoint getScreenCoords(int16_t x, int16_t y)
{
    int16_t xCoord = round((x * x_touch_touchConfig.xCalM) + x_touch_touchConfig.xCalC);
    int16_t yCoord = round((y * x_touch_touchConfig.yCalM) + x_touch_touchConfig.yCalC);
    if (xCoord < 0)
        xCoord = 0;
    if (xCoord >= 320)
        xCoord = 320 - 1;
    if (yCoord < 0)
        yCoord = 0;
    if (yCoord >= 240)
        yCoord = 240 - 1;
    return (ScreenPoint(xCoord, yCoord));
}

bool xtouch_loadTouchConfig(XTouchPanelConfig &config)
{
    ConsoleError.println(F("[XTouch][Touch] Starting load config"));
    
    // Initialize with safe defaults in case of failure
    config.xCalM = 1.0;
    config.yCalM = 1.0;
    config.xCalC = 0.0;
    config.yCalC = 0.0;
    
    // Open file for reading
    File file = xtouch_filesystem_open(SD, xtouch_paths_touch);
    if (!file)
    {
        ConsoleError.println(F("[XTouch][Touch] Failed to open file"));
        return false;
    }

    ConsoleError.println(F("[XTouch][Touch] File opened"));
    
    // Read file into a String first to avoid direct parsing issues
    String jsonString = "";
    while (file.available()) {
        jsonString += (char)file.read();
    }
    file.close();
    
    ConsoleError.println(F("[XTouch][Touch] File read, size: "));
    ConsoleError.println(jsonString.length());
    ConsoleError.println(jsonString.c_str());
    
    // Use a larger buffer to be safe
    DynamicJsonDocument doc(1024);
    
    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, jsonString);
    
    if (error)
    {
        ConsoleError.println(F("[XTouch][Touch] JSON parse error: "));
        ConsoleError.println(error.c_str());
        return false;
    }
    
    ConsoleError.println(F("[XTouch][Touch] JSON parsed successfully"));
    
    // Check if all required fields exist and are the correct type
    if (!doc.containsKey("xCalM") || !doc["xCalM"].is<float>()) {
        ConsoleError.println(F("[XTouch][Touch] Invalid xCalM field"));
        return false;
    }
    
    if (!doc.containsKey("yCalM") || !doc["yCalM"].is<float>()) {
        ConsoleError.println(F("[XTouch][Touch] Invalid yCalM field"));
        return false;
    }
    
    if (!doc.containsKey("xCalC") || !doc["xCalC"].is<float>()) {
        ConsoleError.println(F("[XTouch][Touch] Invalid xCalC field"));
        return false;
    }
    
    if (!doc.containsKey("yCalC") || !doc["yCalC"].is<float>()) {
        ConsoleError.println(F("[XTouch][Touch] Invalid yCalC field"));
        return false;
    }
    
    // All fields exist and are valid, now read them
    ConsoleError.println(F("[XTouch][Touch] Reading values"));
    config.xCalM = doc["xCalM"].as<float>();
    config.yCalM = doc["yCalM"].as<float>();
    config.xCalC = doc["xCalC"].as<float>();
    config.yCalC = doc["yCalC"].as<float>();

    ConsoleError.println(F("[XTouch][Touch] Config loaded successfully"));
    return true;
}

void xtouch_saveTouchConfig(XTouchPanelConfig &config)
{
    StaticJsonDocument<512> doc;
    doc["xCalM"] = config.xCalM;
    doc["yCalM"] = config.yCalM;
    doc["xCalC"] = config.xCalC;
    doc["yCalC"] = config.yCalC;
    xtouch_filesystem_writeJson(SD, xtouch_paths_touch, doc);
}

void xtouch_resetTouchConfig()
{
    ConsoleInfo.println(F("[XTouch][FS] Resetting touch config"));
    xtouch_filesystem_deleteFile(SD, xtouch_paths_touch);
    delay(500);
    ESP.restart();
}

bool hasTouchConfig()
{
    ConsoleInfo.println(F("[XTouch][FS] Checking for touch config"));
    return xtouch_filesystem_exist(SD, xtouch_paths_touch);
}

void xtouch_touch_setup()
{
    if (hasTouchConfig())
    {
        ConsoleInfo.println(F("[XTouch][TOUCH] Load from disk..."));
        xtouch_loadTouchConfig(x_touch_touchConfig);
        ConsoleInfo.println(F("[XTouch][TOUCH] ...Done"));
        delay(50);
    }
    else
    {
        ConsoleInfo.println(F("[XTouch][TOUCH] Touch Setup"));
        TS_Point p;
        int16_t x1, y1, x2, y2;

        lv_label_set_text(introScreenCaption, "Touch the  " LV_SYMBOL_PLUS "  with the stylus");
        lv_timer_handler();

        // wait for no touch
        while (x_touch_touchScreen.touched())
            ;
        tft.drawFastHLine(0, 10, 20, ILI9341_WHITE);
        tft.drawFastVLine(10, 0, 20, ILI9341_WHITE);
        while (!x_touch_touchScreen.touched())
            ;
        delay(50);
        p = x_touch_touchScreen.getPoint();
        x1 = p.x;
        y1 = p.y;
        tft.drawFastHLine(0, 10, 20, ILI9341_BLACK);
        tft.drawFastVLine(10, 0, 20, ILI9341_BLACK);
        delay(500);

        while (x_touch_touchScreen.touched())
            ;
        tft.drawFastHLine(300, 230, 20, ILI9341_WHITE);
        tft.drawFastVLine(310, 220, 20, ILI9341_WHITE);

        while (!x_touch_touchScreen.touched())
            ;
        delay(50);
        p = x_touch_touchScreen.getPoint();
        x2 = p.x;
        y2 = p.y;
        tft.drawFastHLine(300, 230, 20, ILI9341_BLACK);
        tft.drawFastVLine(310, 220, 20, ILI9341_BLACK);

        int16_t xDist = 320 - 40;
        int16_t yDist = 240 - 40;

        x_touch_touchConfig.xCalM = (float)xDist / (float)(x2 - x1);
        x_touch_touchConfig.xCalC = 20.0 - ((float)x1 * x_touch_touchConfig.xCalM);
        // y
        x_touch_touchConfig.yCalM = (float)yDist / (float)(y2 - y1);
        x_touch_touchConfig.yCalC = 20.0 - ((float)y1 * x_touch_touchConfig.yCalM);

        xtouch_saveTouchConfig(x_touch_touchConfig);

        loadScreen(-1);
    }
}

#endif